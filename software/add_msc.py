"""
PlatformIO extra_script – injects STM32 USB MSC middleware sources into the build.
Called automatically by PlatformIO before building because of the
  extra_scripts = add_msc.py
line in platformio.ini.
"""
Import("env")
import os, glob

# ── Locate framework-arduinoststm32 ──────────────────────────────────────────
pio_home = os.path.join(os.path.expanduser("~"), ".platformio")
candidates = glob.glob(os.path.join(pio_home, "packages", "framework-arduinoststm32*"))
if not candidates:
    print("[add_msc] ERROR: framework-arduinoststm32 not found in", pio_home)
    Exit(1)
fw_dir = candidates[0]

# ── Paths ─────────────────────────────────────────────────────────────────────
msc_base = os.path.join(fw_dir, "system", "Middlewares", "ST",
                         "STM32_USB_Device_Library", "Class", "MSC")
core_inc  = os.path.join(fw_dir, "system", "Middlewares", "ST",
                         "STM32_USB_Device_Library", "Core", "Inc")
msc_inc   = os.path.join(msc_base, "Inc")
msc_src   = os.path.join(msc_base, "Src")

# ── Add include paths ─────────────────────────────────────────────────────────
# Prepend so our Inc/ (which shadows usbd_ep_conf.h) is searched FIRST.
project_inc = os.path.join(env.subst("$PROJECT_DIR"), "Inc")
env.Prepend(CPPPATH=[project_inc, msc_inc, core_inc])

# ── Exclude framework's usbd_ep_conf.c (we provide Src/usbd_ep_conf.c) ────────
fw_ep_conf = os.path.normpath(
    os.path.join(fw_dir, "libraries", "USBDevice", "src", "usbd_ep_conf.c"))

def _skip_framework_ep_conf(env, node):
    if os.path.normpath(str(node)) == fw_ep_conf:
        print("[add_msc] Excluded framework usbd_ep_conf.c (using project version)")
        return None
    return node

env.AddBuildMiddleware(_skip_framework_ep_conf, "*.c")

# ── Compile MSC middleware source files ───────────────────────────────────────
msc_sources = [
    "usbd_msc.c",
    "usbd_msc_bot.c",
    "usbd_msc_scsi.c",
    "usbd_msc_data.c",
]

objects = []
for fname in msc_sources:
    fpath = os.path.join(msc_src, fname)
    if os.path.exists(fpath):
        objects.append(env.Object(source=fpath))
    else:
        print("[add_msc] WARNING: not found:", fpath)

env.Append(PIOBUILDFILES=objects)
print("[add_msc] Added", len(objects), "MSC middleware objects to build.")
