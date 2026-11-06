"""
PlatformIO pre-script: link outputs use project name instead of firmware.*.
Must match scripts/post_build_sd_apps.py expectations (music_player.bin on SD).
"""
Import("env")

env.Replace(PROGNAME="music_player")
