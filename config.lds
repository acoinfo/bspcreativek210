MEMORY
{
  TEXT (rx) : ORIGIN = (0x80000000), LENGTH = (2 * 1024 * 1024)
  DATA (rw) : ORIGIN = (0x80000000) + ((2 * 1024 * 1024)), LENGTH = (4 * 1024 * 1024)
}
BOOT_STACK_SIZE = (64 * 1024);
