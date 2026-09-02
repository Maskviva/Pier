//! `hello-pier` —— 契约 §十「加一门语言」四步的**可运行**参考。
//!
//! 这个示例刻意只用运行时地基（握手、日志、生命周期），一个域 API 都不碰。
//! 理由：它要验证的是**契约本身通不通**，而不是某个域封装好不好用。
//! 一个只用地基的模组能装上、能打日志、能被卸载，就说明四步都对了：
//!
//! 1. 只读 `sdk/abi.h`（这里经由 `levilamina_sys` 那份手写镜像）；
//! 2. 无条件声明整张表（镜像已做，且 `sys-mirrors-abi` 守着）；
//! 3. 导出 `pier_main`，填好 `struct_size / abi_version / mod_flags`
//!    和三个生命周期回调（`register_mod!` 展开出来的就是这个）；
//! 4. 每个非核心槽调用前查两道闸（这里用 `host_abi()` 演示怎么读宿主能力）。
//!
//! 装上之后控制台应该出现三行 `[hello-pier]`：装载、启用、卸载各一行。
//! 少任何一行都说明对应那一段生命周期没走通 —— 这就是这个示例的用处。

use levilamina::prelude::*;

struct HelloPier {
    /// 记一下启用了几次。热重载会让 enable/disable 成对多次发生，
    /// 打出来能看出宿主的 reload 有没有真的走完一轮。
    enabled_times: u32,
}

impl LeviMod for HelloPier {
    fn on_load(ctx: &ModContext) -> Result<Self> {
        let (abi, table_len) = ctx.host_abi();
        ctx.logger().info(&format!(
            "装载成功。宿主 ABI v{abi}，函数表 {table_len} 字节，\
             目标={}。",
            if ctx.host_is_client() {
                "客户端"
            } else {
                "服务端"
            }
        ));

        // 契约 §十 第 4 步的演示：非核心槽调用前查两道闸。
        // `has_slot!` 同时查「表够不够长」和「槽是不是空」——
        // 前者防越界读，后者防那个能力包没编进宿主。
        if levilamina::has_slot!(md_is_available) {
            ctx.logger()
                .info("这个宿主编入了自定义维度能力（md_* 槽非空）。");
        } else {
            // 这不是错误，是**刻意的降级**：pier-dimensions 是可选包
            // （契约 §一 规则三），不编入时槽位为 NULL。
            ctx.logger()
                .info("这个宿主没有编入自定义维度能力 —— 槽位为空，按不支持处理。");
        }

        Ok(HelloPier { enabled_times: 0 })
    }

    fn on_enable(&mut self, ctx: &ModContext) -> Result<()> {
        self.enabled_times += 1;
        let host = Host::get();
        ctx.logger().info(&format!(
            "启用（第 {} 次）。服务器阶段={:?}，在线 {} 人，tick {}。",
            self.enabled_times,
            host.gaming_status(),
            host.player_count()
                .map_or("?".to_owned(), |n| n.to_string()),
            host.current_tick()
                .map_or("?".to_owned(), |t| t.to_string()),
        ));

        // 契约 §5.3 的演示：订阅/解析失败时，把宿主**认得的** id 列出来，
        // 比一句「失败」有用得多。这里只打个数，真订阅时应该打相近的几条。
        let events = host.list_events();
        ctx.logger()
            .info(&format!("宿主当前能解析 {} 个事件 id。", events.len()));

        // 把一段活儿丢回服务器线程。闭包被装箱交给宿主，跑完立刻释放 ——
        // 所有权全程在模组这一侧（契约 §三）。
        //
        // 返回的是**票据**：这条路走的是带模组句柄的 `schedule_for`，宿主按模组
        // 记账，卸载时会把没跑完的整批丢掉（旧的无主槽做不到这一点，定时器会在
        // 模组卸载后跳进已经 unmap 的代码段）。想提前取消就 `host.cancel(task)`。
        let task = host.schedule_after(std::time::Duration::from_secs(1), || {
            Logger::get().info("一秒后的排期任务跑到了。");
        })?;
        ctx.logger().info(&format!(
            "排期票据 {}；本模组名下待执行 {} 个。",
            task.raw(),
            host.pending_tasks()
        ));
        Ok(())
    }

    fn on_disable(&mut self, ctx: &ModContext) -> Result<()> {
        ctx.logger().info("停用。");
        Ok(())
    }

    fn on_unload(&mut self, ctx: &ModContext) -> Result<()> {
        // 这里还拿得到 `&mut self`，收尾工作放这儿。宿主在这个回调返回之后
        // 才会跑拆除步骤并最终 FreeLibrary。
        ctx.logger()
            .info(&format!("卸载。本次会话共启用 {} 次。", self.enabled_times));
        Ok(())
    }
}

levilamina::register_mod!(HelloPier);
