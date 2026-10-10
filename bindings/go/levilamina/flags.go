//go:build !pier_client

package levilamina

// modFlags is the target this DLL declares: the server. Build with -tags pier_client for
// the client host, which refuses a mismatch at load.
const modFlags uint32 = 0
