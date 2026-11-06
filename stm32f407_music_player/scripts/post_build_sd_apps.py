Import("env")

APP_FLASH_BASE = 0x08010000
# Must match boot_app.c APP_RESET_MAX_OFFSET in stm32f407-app-selector
APP_RESET_MAX_OFFSET = 48 * 1024
SD_APP_NAME = "music_player.bin"


def _u32_le(data, offset):
    return data[offset] | (data[offset + 1] << 8) | (data[offset + 2] << 16) | (data[offset + 3] << 24)


def _verify_app_bin(bin_path):
    with open(bin_path, "rb") as f:
        data = f.read(8)
    if len(data) < 8:
        raise RuntimeError("%s is too small to contain a vector table" % bin_path)

    reset = _u32_le(data, 4) & ~1
    if reset < APP_FLASH_BASE:
        raise RuntimeError(
            "Reset vector 0x%08X looks like a 0x08000000-linked image; expected >= 0x%08X"
            % (reset, APP_FLASH_BASE)
        )
    reset_off = reset - APP_FLASH_BASE
    if reset_off >= APP_RESET_MAX_OFFSET:
        raise RuntimeError(
            "Reset vector 0x%08X is +0x%X from app base; bootloader allows < 0x%X "
            "(link Reset_Handler near vector table — see STM32F407VGTX_APP.ld)"
            % (reset, reset_off, APP_RESET_MAX_OFFSET)
        )
    print(
        "App image OK: flash 0x%08X, reset 0x%08X (+0x%X, bootloader check passed)"
        % (APP_FLASH_BASE, reset, reset_off)
    )


def export_app_images(target, source, env):
    import os
    import shutil
    import subprocess

    build_dir = env.subst("$BUILD_DIR")
    project_dir = env.subst("$PROJECT_DIR")
    progname = env.subst("${PROGNAME}")
    objcopy = env.subst("$OBJCOPY")

    # SCons action signature: (target, source, env)
    bin_path = str(target[0])
    elf_path = str(source[0])
    bin_named = os.path.join(build_dir, "%s_0x%08X.bin" % (progname, APP_FLASH_BASE))
    hex_named = os.path.join(build_dir, "%s_0x%08X.hex" % (progname, APP_FLASH_BASE))

    subprocess.check_call([objcopy, "-O", "binary", elf_path, bin_named])
    subprocess.check_call([objcopy, "-O", "ihex", elf_path, hex_named])
    _verify_app_bin(bin_named)

    if os.path.abspath(bin_path) != os.path.abspath(bin_named):
        shutil.copy2(bin_named, bin_path)

    dst_dir = os.path.join(project_dir, "sd_card", "apps")
    os.makedirs(dst_dir, exist_ok=True)
    shutil.copy2(bin_named, os.path.join(dst_dir, SD_APP_NAME))
    shutil.copy2(hex_named, os.path.join(dst_dir, "music_player_0x08010000.hex"))

    print("")
    print("Raw .bin has NO address in the file — programmers default to 0x08000000.")
    print("  SD bootloader: copy sd_card/apps/%s -> microSD /apps/ (writes at 0x08010000)." % SD_APP_NAME)
    print("  ST-Link / CubeProgrammer: use %s OR set start address 0x08010000." % hex_named)
    print("")


env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", export_app_images)

env.AddCustomTarget(
    name="appimage",
    dependencies="$BUILD_DIR/${PROGNAME}.bin",
    actions=env.Action(export_app_images, "Dump app .bin/.hex for flash base 0x08010000"),
    title="App image @ 0x08010000",
)
