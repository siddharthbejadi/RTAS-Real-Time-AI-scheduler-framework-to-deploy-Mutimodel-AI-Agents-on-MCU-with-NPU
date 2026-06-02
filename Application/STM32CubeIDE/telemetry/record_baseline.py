import argparse
import csv
import os
import re
import time
from datetime import datetime
from pathlib import Path

import serial
from serial.tools import list_ports


BAUD_RATE = 115200
DEFAULT_OUTPUT = "after_double_buffer_implemented.csv"
STLINK_KEYWORDS = ("STMicroelectronics", "STLink", "ST-Link", "Virtual COM Port")

FIELDNAMES = [
    "Run_ID",
    "Sample_Index",
    "Timestamp",
    "Timestamp_ISO",
    "Serial_Port",
    "Baud_Rate",
    "Telemetry_Format",
    "Raw_Line",
    "Frames_Per_Sample",
    "Firmware_Frames",
    "Avg_NPU_us",
    "Avg_Detect_NPU_us",
    "Avg_Embed_NPU_us",
    "Avg_Depth_NPU_us",
    "Avg_CPU_Active_us",
    "Avg_CPU_Sleep_us",
    "Avg_Total_us",
    "Avg_Total_ms",
    "Inference_Latency_ms",
    "NPU_Latency_ms",
    "CPU_Active_ms",
    "CPU_Sleep_ms",
    "Throughput_FPS",
    "CPU_Utilization_pct",
    "NPU_Utilization_pct",
    "Efficiency_Inferences_Per_Second",
    "Efficiency_MACs_Per_Second",
    "Efficiency_FLOPs_Per_Second",
    "Inferences_Per_Joule",
    "Energy_Per_Inference_mJ",
    "Avg_Power_mW",
    "Voltage_V",
    "Current_mA",
    "Battery_Voltage_V",
    "Battery_Percent",
    "CPU_Clock_MHz",
    "NPU_Clock_MHz",
    "HCLK_Hz",
    "PCLK1_Hz",
    "PCLK2_Hz",
    "SRAM_Used_Bytes",
    "SRAM_Total_Bytes",
    "Flash_Used_Bytes",
    "Flash_Total_Bytes",
    "External_RAM_Used_Bytes",
    "External_RAM_Total_Bytes",
    "VRAM_Used_Bytes",
    "VRAM_Total_Bytes",
    "Model_Name",
    "Model_Size_Bytes",
    "Model_Artifact_Bytes",
    "MACs",
    "FLOPs",
    "NN_Input_Bytes",
    "NN_Output_Bytes",
    "NN_Context_Bytes",
    "Camera_Buffer_Bytes",
    "Crop_Buffer_Bytes",
    "Schedule_Flags",
]

ALIASES = {
    "frames": "Firmware_Frames",
    "frame_count": "Firmware_Frames",
    "firmware_frames": "Firmware_Frames",
    "avg_npu_us": "Avg_NPU_us",
    "npu_us": "Avg_NPU_us",
    "avg_npu_total_us": "Avg_NPU_us",
    "avg_detect_npu_us": "Avg_Detect_NPU_us",
    "detect_npu_us": "Avg_Detect_NPU_us",
    "avg_embed_npu_us": "Avg_Embed_NPU_us",
    "embed_npu_us": "Avg_Embed_NPU_us",
    "avg_depth_npu_us": "Avg_Depth_NPU_us",
    "depth_npu_us": "Avg_Depth_NPU_us",
    "avg_cpu_active_us": "Avg_CPU_Active_us",
    "cpu_active_us": "Avg_CPU_Active_us",
    "active_us": "Avg_CPU_Active_us",
    "avg_cpu_sleep_us": "Avg_CPU_Sleep_us",
    "cpu_sleep_us": "Avg_CPU_Sleep_us",
    "sleep_us": "Avg_CPU_Sleep_us",
    "avg_total_us": "Avg_Total_us",
    "total_us": "Avg_Total_us",
    "epoch_us": "Avg_Total_us",
    "avg_total_ms": "Avg_Total_ms",
    "total_ms": "Avg_Total_ms",
    "latency_ms": "Inference_Latency_ms",
    "inference_latency_ms": "Inference_Latency_ms",
    "npu_latency_ms": "NPU_Latency_ms",
    "fps": "Throughput_FPS",
    "throughput_fps": "Throughput_FPS",
    "cpu_pct": "CPU_Utilization_pct",
    "cpu_util_pct": "CPU_Utilization_pct",
    "cpu_utilization_pct": "CPU_Utilization_pct",
    "npu_pct": "NPU_Utilization_pct",
    "npu_run_pct": "NPU_Utilization_pct",
    "npu_util_pct": "NPU_Utilization_pct",
    "npu_utilization_pct": "NPU_Utilization_pct",
    "power_mw": "Avg_Power_mW",
    "avg_power_mw": "Avg_Power_mW",
    "voltage_v": "Voltage_V",
    "current_ma": "Current_mA",
    "battery_voltage_v": "Battery_Voltage_V",
    "battery_percent": "Battery_Percent",
    "cpu_clock_mhz": "CPU_Clock_MHz",
    "npu_clock_mhz": "NPU_Clock_MHz",
    "hclk_hz": "HCLK_Hz",
    "pclk1_hz": "PCLK1_Hz",
    "pclk2_hz": "PCLK2_Hz",
    "sram_used_bytes": "SRAM_Used_Bytes",
    "sram_total_bytes": "SRAM_Total_Bytes",
    "flash_used_bytes": "Flash_Used_Bytes",
    "flash_total_bytes": "Flash_Total_Bytes",
    "external_ram_used_bytes": "External_RAM_Used_Bytes",
    "external_ram_total_bytes": "External_RAM_Total_Bytes",
    "vram_used_bytes": "VRAM_Used_Bytes",
    "vram_total_bytes": "VRAM_Total_Bytes",
    "model_name": "Model_Name",
    "model_size_bytes": "Model_Size_Bytes",
    "model_artifact_bytes": "Model_Artifact_Bytes",
    "mac": "MACs",
    "macs": "MACs",
    "macc": "MACs",
    "flop": "FLOPs",
    "flops": "FLOPs",
    "nn_input_bytes": "NN_Input_Bytes",
    "nn_output_bytes": "NN_Output_Bytes",
    "nn_context_bytes": "NN_Context_Bytes",
    "camera_buffer_bytes": "Camera_Buffer_Bytes",
    "crop_buffer_bytes": "Crop_Buffer_Bytes",
    "schedule_flags": "Schedule_Flags",
}


def get_ports():
    return list(list_ports.comports())


def describe_port(port):
    return f"{port.device}: {port.description} [{port.hwid}]"


def find_stlink_port():
    for port in get_ports():
        details = " ".join(
            str(value)
            for value in (
                port.description,
                port.manufacturer,
                port.product,
                port.hwid,
            )
            if value
        )
        if any(keyword.lower() in details.lower() for keyword in STLINK_KEYWORDS):
            return port.device
    return None


def print_ports():
    ports = get_ports()
    if not ports:
        print("No serial ports found.")
        return

    print("Available serial ports:")
    for port in ports:
        print(f"  {describe_port(port)}")


def default_model_dir():
    try:
        project_root = Path(__file__).resolve().parents[3]
    except IndexError:
        return None

    candidate = project_root / "Model" / "STM32N6570-DK"
    return candidate if candidate.exists() else None


def estimate_model_artifact_bytes(model_dir):
    if not model_dir:
        return None

    model_path = Path(model_dir)
    if not model_path.exists():
        return None

    total = 0
    for path in model_path.glob("*_atonbuf*.bin"):
        total += path.stat().st_size

    return total or None


def normalize_key(value):
    return re.sub(r"[^a-z0-9]+", "_", value.strip().lower()).strip("_")


def clean_value(value):
    return value.strip().strip('"')


def as_float(value):
    if value in (None, ""):
        return None

    text = str(value).strip().replace(",", "")
    match = re.search(r"-?\d+(?:\.\d+)?", text)
    if not match:
        return None

    try:
        return float(match.group(0))
    except ValueError:
        return None


def set_if_missing(row, key, value):
    if value is None or row.get(key) not in (None, ""):
        return

    if isinstance(value, float):
        row[key] = f"{value:.6g}"
    else:
        row[key] = value


def parse_telemetry_line(line):
    if not line.startswith("TELEMETRY"):
        return None

    parts = [part.strip() for part in line.split(",")]
    payload = parts[1:]

    row = {field: "" for field in FIELDNAMES}
    row["Raw_Line"] = line

    if any("=" in part for part in payload):
        row["Telemetry_Format"] = "key_value"
        for part in payload:
            if "=" not in part:
                continue

            key, value = part.split("=", 1)
            column = ALIASES.get(normalize_key(key))
            if column:
                row[column] = clean_value(value)
        return row

    row["Telemetry_Format"] = "legacy_csv"
    if len(payload) < 4:
        return None

    row["Avg_NPU_us"] = clean_value(payload[0])
    row["Avg_CPU_Active_us"] = clean_value(payload[1])
    row["Avg_CPU_Sleep_us"] = clean_value(payload[2])
    row["Avg_Total_ms"] = clean_value(payload[3])
    return row


def enrich_row(row, args, serial_port, sample_index, model_artifact_bytes):
    now = datetime.now()

    row["Run_ID"] = args.run_id
    row["Sample_Index"] = sample_index
    row["Timestamp"] = now.strftime("%H:%M:%S")
    row["Timestamp_ISO"] = now.isoformat(timespec="seconds")
    row["Serial_Port"] = serial_port
    row["Baud_Rate"] = args.baud

    set_if_missing(row, "Frames_Per_Sample", args.frames_per_sample)
    set_if_missing(row, "Firmware_Frames", args.frames_per_sample)
    set_if_missing(row, "Avg_Power_mW", args.power_mw)
    set_if_missing(row, "Voltage_V", args.voltage_v)
    set_if_missing(row, "Current_mA", args.current_ma)
    set_if_missing(row, "Battery_Voltage_V", args.battery_voltage_v)
    set_if_missing(row, "Battery_Percent", args.battery_percent)
    set_if_missing(row, "CPU_Clock_MHz", args.cpu_clock_mhz)
    set_if_missing(row, "NPU_Clock_MHz", args.npu_clock_mhz)
    set_if_missing(row, "SRAM_Used_Bytes", args.sram_used_bytes)
    set_if_missing(row, "SRAM_Total_Bytes", args.sram_total_bytes)
    set_if_missing(row, "Flash_Used_Bytes", args.flash_used_bytes)
    set_if_missing(row, "Flash_Total_Bytes", args.flash_total_bytes)
    set_if_missing(row, "External_RAM_Used_Bytes", args.external_ram_used_bytes)
    set_if_missing(row, "External_RAM_Total_Bytes", args.external_ram_total_bytes)
    set_if_missing(row, "VRAM_Used_Bytes", args.vram_used_bytes)
    set_if_missing(row, "VRAM_Total_Bytes", args.vram_total_bytes)
    set_if_missing(row, "Model_Name", args.model_name)
    set_if_missing(row, "Model_Artifact_Bytes", model_artifact_bytes)
    set_if_missing(row, "Model_Size_Bytes", args.model_size_bytes or model_artifact_bytes)
    set_if_missing(row, "MACs", args.macs)
    set_if_missing(row, "FLOPs", args.flops)

    avg_npu_us = as_float(row.get("Avg_NPU_us"))
    avg_active_us = as_float(row.get("Avg_CPU_Active_us"))
    avg_sleep_us = as_float(row.get("Avg_CPU_Sleep_us"))
    avg_total_us = as_float(row.get("Avg_Total_us"))
    avg_total_ms = as_float(row.get("Avg_Total_ms"))
    voltage_v = as_float(row.get("Voltage_V"))
    current_ma = as_float(row.get("Current_mA"))
    power_mw = as_float(row.get("Avg_Power_mW"))
    macs = as_float(row.get("MACs"))
    flops = as_float(row.get("FLOPs"))

    if avg_total_us is None and avg_total_ms is not None:
        avg_total_us = avg_total_ms * 1000.0
        set_if_missing(row, "Avg_Total_us", avg_total_us)
    if avg_total_ms is None and avg_total_us is not None:
        avg_total_ms = avg_total_us / 1000.0
        set_if_missing(row, "Avg_Total_ms", avg_total_ms)

    set_if_missing(row, "Inference_Latency_ms", avg_total_ms)
    if avg_npu_us is not None:
        set_if_missing(row, "NPU_Latency_ms", avg_npu_us / 1000.0)
    if avg_active_us is not None:
        set_if_missing(row, "CPU_Active_ms", avg_active_us / 1000.0)
    if avg_sleep_us is not None:
        set_if_missing(row, "CPU_Sleep_ms", avg_sleep_us / 1000.0)

    latency_ms = as_float(row.get("Inference_Latency_ms"))
    if latency_ms and latency_ms > 0:
        throughput_fps = 1000.0 / latency_ms
        set_if_missing(row, "Throughput_FPS", throughput_fps)
        set_if_missing(row, "Efficiency_Inferences_Per_Second", throughput_fps)

        latency_s = latency_ms / 1000.0
        if macs is not None:
            set_if_missing(row, "Efficiency_MACs_Per_Second", macs / latency_s)
        if flops is not None:
            set_if_missing(row, "Efficiency_FLOPs_Per_Second", flops / latency_s)

    active_plus_sleep = None
    if avg_active_us is not None and avg_sleep_us is not None:
        active_plus_sleep = avg_active_us + avg_sleep_us

    cpu_denominator = avg_total_us or active_plus_sleep
    if avg_active_us is not None and cpu_denominator and cpu_denominator > 0:
        set_if_missing(row, "CPU_Utilization_pct", (avg_active_us * 100.0) / cpu_denominator)

    if avg_npu_us is not None and avg_total_us and avg_total_us > 0:
        set_if_missing(row, "NPU_Utilization_pct", (avg_npu_us * 100.0) / avg_total_us)

    if power_mw is None and voltage_v is not None and current_ma is not None:
        power_mw = voltage_v * current_ma
        set_if_missing(row, "Avg_Power_mW", power_mw)

    if power_mw is not None and latency_ms and latency_ms > 0:
        energy_mj = (power_mw * latency_ms) / 1000.0
        set_if_missing(row, "Energy_Per_Inference_mJ", energy_mj)
        if energy_mj > 0:
            set_if_missing(row, "Inferences_Per_Joule", 1000.0 / energy_mj)

    hclk_hz = as_float(row.get("HCLK_Hz"))
    if hclk_hz is not None:
        set_if_missing(row, "CPU_Clock_MHz", hclk_hz / 1000000.0)

    return row


def parse_args():
    model_dir = default_model_dir()

    parser = argparse.ArgumentParser(
        description="Record STM32 TELEMETRY lines to a graph-ready CSV file."
    )
    parser.add_argument(
        "-p",
        "--port",
        help="Serial port to open, for example COM23. If omitted, the STLink VCP is auto-detected.",
    )
    parser.add_argument(
        "-b",
        "--baud",
        type=int,
        default=BAUD_RATE,
        help=f"Serial baud rate. Default: {BAUD_RATE}.",
    )
    parser.add_argument(
        "-o",
        "--output",
        default=DEFAULT_OUTPUT,
        help=f"CSV output path. Default: {DEFAULT_OUTPUT}.",
    )
    parser.add_argument("--append", action="store_true", help="Append to an existing CSV.")
    parser.add_argument(
        "--duration",
        type=float,
        help="Optional number of seconds to record before exiting.",
    )
    parser.add_argument(
        "--frames-per-sample",
        type=int,
        default=100,
        help="Number of frames averaged by each firmware telemetry sample. Default: 100.",
    )
    parser.add_argument(
        "--run-id",
        default=datetime.now().strftime("%Y%m%d_%H%M%S"),
        help="Identifier written to every row so multiple runs can be compared.",
    )
    parser.add_argument(
        "--model-dir",
        default=str(model_dir) if model_dir else "",
        help="Model artifact directory used to estimate Model_Artifact_Bytes.",
    )
    parser.add_argument("--model-name", default="", help="Model or experiment name.")
    parser.add_argument("--model-size-bytes", type=float, help="Model size in bytes.")
    parser.add_argument("--macs", type=float, help="MAC count per inference.")
    parser.add_argument("--flops", type=float, help="FLOP count per inference.")
    parser.add_argument("--power-mw", type=float, help="Average power in milliwatts.")
    parser.add_argument("--voltage-v", type=float, help="Measured supply voltage.")
    parser.add_argument("--current-ma", type=float, help="Measured current in milliamps.")
    parser.add_argument("--battery-voltage-v", type=float, help="Battery voltage.")
    parser.add_argument("--battery-percent", type=float, help="Battery percentage.")
    parser.add_argument("--cpu-clock-mhz", type=float, help="CPU clock in MHz.")
    parser.add_argument("--npu-clock-mhz", type=float, help="NPU clock in MHz.")
    parser.add_argument("--sram-used-bytes", type=float, help="Used SRAM bytes.")
    parser.add_argument("--sram-total-bytes", type=float, help="Total SRAM bytes.")
    parser.add_argument("--flash-used-bytes", type=float, help="Used flash bytes.")
    parser.add_argument("--flash-total-bytes", type=float, help="Total flash bytes.")
    parser.add_argument("--external-ram-used-bytes", type=float, help="Used external RAM bytes.")
    parser.add_argument("--external-ram-total-bytes", type=float, help="Total external RAM bytes.")
    parser.add_argument("--vram-used-bytes", type=float, help="Used video/external frame memory bytes.")
    parser.add_argument("--vram-total-bytes", type=float, help="Total video/external frame memory bytes.")
    parser.add_argument(
        "--show-all-lines",
        action="store_true",
        help="Print non-telemetry serial lines too. Useful for debugging firmware output.",
    )
    parser.add_argument(
        "--list-ports",
        action="store_true",
        help="Show detected serial ports and exit.",
    )
    return parser.parse_args()


def main():
    args = parse_args()

    if args.list_ports:
        print_ports()
        return

    serial_port = args.port or find_stlink_port()
    if not serial_port:
        print("Error: could not auto-detect an STLink virtual COM port.")
        print_ports()
        print("\nRun again with --port COMx using the STM32 port shown above.")
        return

    model_artifact_bytes = estimate_model_artifact_bytes(args.model_dir)
    print(f"Connecting to {serial_port} at {args.baud} baud...")

    start_time = time.monotonic()
    rows_logged = 0
    csv_mode = "a" if args.append else "w"
    write_header = not args.append or not os.path.exists(args.output) or os.path.getsize(args.output) == 0

    try:
        with serial.Serial(serial_port, args.baud, timeout=1) as ser:
            with open(args.output, mode=csv_mode, newline="") as file:
                writer = csv.DictWriter(file, fieldnames=FIELDNAMES)
                if write_header:
                    writer.writeheader()

                print(f"Recording data to {args.output}... Press Ctrl+C to stop.")

                while True:
                    if args.duration and time.monotonic() - start_time >= args.duration:
                        break

                    line = ser.readline().decode("utf-8", errors="replace").strip()
                    if not line:
                        continue

                    row = parse_telemetry_line(line)
                    if row is None:
                        if args.show_all_lines:
                            print(line)
                        continue

                    rows_logged += 1
                    row = enrich_row(row, args, serial_port, rows_logged, model_artifact_bytes)
                    writer.writerow(row)
                    file.flush()

                    print(
                        "Logged sample "
                        f"{rows_logged}: latency={row.get('Inference_Latency_ms', '')} ms, "
                        f"npu={row.get('Avg_NPU_us', '')} us, "
                        f"fps={row.get('Throughput_FPS', '')}"
                    )

    except KeyboardInterrupt:
        print("\nRecording stopped.")
    except serial.SerialException as exc:
        print(f"Error: could not open/read {serial_port}: {exc}")
        print("\nTry these quick checks:")
        print("  1. Close STM32CubeIDE serial monitor, PuTTY, Tera Term, or any other terminal.")
        print("  2. Unplug/replug the board, then run: python record_baseline.py --list-ports")
        print(f"  3. Run explicitly with: python record_baseline.py --port {serial_port}")
    else:
        print("Recording finished.")
    finally:
        print(f"Rows logged: {rows_logged}")


if __name__ == "__main__":
    main()
