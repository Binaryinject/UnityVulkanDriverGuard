#!/usr/bin/env python3
"""Generate DriverGuard.ini selectors from the pci.ids database.

The PCI database identifies hardware; it does not establish driver
compatibility.  Deny rules are therefore opt-in through --*-deny-below.
"""

from __future__ import annotations

import argparse
import re
import urllib.request
from pathlib import Path

PCI_IDS_URL = "https://raw.githubusercontent.com/pciutils/pciids/master/pci.ids"
VENDORS = {
    "nvidia": ("10de", "GPU_NVIDIA", "NVIDIA_PROPRIETARY", "https://www.nvidia.com/Download/index.aspx"),
    "amd": ("1002", "GPU_AMD", "AMD_PROPRIETARY,AMD_OPEN_SOURCE,MESA_RADV", "https://www.amd.com/en/support/download/drivers.html"),
    "intel": ("8086", "GPU_INTEL", "INTEL_PROPRIETARY_WINDOWS,INTEL_OPEN_SOURCE_MESA", "https://www.intel.com/content/www/us/en/download-center/home.html"),
}
GPU_NAME_RE = re.compile(
    r"(?i)(geforce|rtx|gtx|quadro|tesla|nvidia graphics|radeon|firepro|instinct|"
    r"radeon pro|rx [0-9]|arc [a-z]|uhd graphics|iris graphics|hd graphics|iris xe|"
    r"vega graphics|matrox)"
)


def read_pci_ids(path: Path) -> dict[str, list[tuple[str, str]]]:
    vendor_ids = {value[0]: key for key, value in VENDORS.items()}
    current: str | None = None
    devices: dict[str, list[tuple[str, str]]] = {key: [] for key in VENDORS}
    vendor_re = re.compile(r"^([0-9a-fA-F]{4})  (.+)$")
    device_re = re.compile(r"^\s{1,8}([0-9a-fA-F]{4})  (.+)$")
    for line in path.read_text(encoding="utf-8", errors="replace").splitlines():
        vendor = vendor_re.match(line)
        if vendor:
            current = vendor_ids.get(vendor.group(1).lower())
            continue
        device = device_re.match(line)
        if current and device and GPU_NAME_RE.search(device.group(2)):
            devices[current].append((device.group(1).upper(), device.group(2).strip()))
    return devices


def quote(value: str) -> str:
    return value.replace("\\", "\\\\").replace('"', '\\"').replace("\r", " ").replace("\n", " ")


def build_ini(devices: dict[str, list[tuple[str, str]]], args: argparse.Namespace) -> str:
    lines = [
        "; Generated from pci.ids by generate-driverguard-ini.py.",
        "; pci.ids identifies hardware; deny thresholds must be compatibility-tested.",
        "[Global]",
        f"MinimumVulkanVersion={args.minimum_vulkan_version}",
    ]
    for key, (_, section, driver_id, download_url) in VENDORS.items():
        entries = devices[key]
        lines += ["", f"[{section}]", f"DownloadURL={download_url}",
                  f"; PCI devices ({len(entries)}): " + ",".join("0x" + item[0] for item in entries)]
        threshold = getattr(args, f"{key}_deny_below")
        if threshold:
            ids = ",".join("0x" + item[0] for item in entries)
            lines.append(
                f'+DriverDenyList=(DriverVersion="<{quote(threshold)}",RHIName="Vulkan",'
                f'DeviceId="{ids}",DriverId="{driver_id}",'
                f'Reason="Known Vulkan compatibility issue; validate before release")'
            )
    return "\n".join(lines) + "\n"


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--pci-ids", type=Path, help="Local pci.ids file; downloads the current file when omitted")
    parser.add_argument("--output", type=Path, required=True, help="Output DriverGuard.ini path")
    parser.add_argument("--minimum-vulkan-version", default="1.1")
    for key in VENDORS:
        parser.add_argument(f"--{key}-deny-below", dest=f"{key}_deny_below",
                            help=f"Opt-in {key} DriverVersion threshold, e.g. 551.76")
    args = parser.parse_args()
    pci_path = args.pci_ids
    temporary = False
    if pci_path is None:
        import tempfile
        handle = tempfile.NamedTemporaryFile(prefix="uvdg-pci-", suffix=".ids", delete=False)
        handle.close()
        pci_path = Path(handle.name)
        temporary = True
        try:
            urllib.request.urlretrieve(PCI_IDS_URL, pci_path)
        except Exception:
            pci_path.unlink(missing_ok=True)
            raise
    try:
        output = build_ini(read_pci_ids(pci_path), args)
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(output, encoding="utf-8", newline="\n")
    finally:
        if temporary:
            pci_path.unlink(missing_ok=True)
    print(f"Generated {args.output}")


if __name__ == "__main__":
    main()
