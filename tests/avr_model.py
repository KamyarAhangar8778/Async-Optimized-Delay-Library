# avr_model.py - Hardware model and timing specifications for AVR MCUs
# Models ATmega8, ATmega16, ATmega32 at 1, 2, 4, 8, 16 MHz clock frequencies.

# MCU Hardware Specifications (RAM & Flash in Bytes)
MCU_SPECS = {
    "ATmega8": {
        "flash_bytes": 8192,
        "ram_bytes": 1024,
        "eeprom_bytes": 512,
        "max_freq_mhz": 16,
        "description": "8KB Flash, 1KB SRAM (32 Pins, Classic AVR)"
    },
    "ATmega16": {
        "flash_bytes": 16384,
        "ram_bytes": 1024,
        "eeprom_bytes": 512,
        "max_freq_mhz": 16,
        "description": "16KB Flash, 1KB SRAM (40 Pins, Standard AVR)"
    },
    "ATmega32": {
        "flash_bytes": 32768,
        "ram_bytes": 2048,
        "eeprom_bytes": 1024,
        "max_freq_mhz": 16,
        "description": "32KB Flash, 2KB SRAM (40 Pins, High-Capacity AVR)"
    }
}

# Supported Operating Frequencies in MHz
SUPPORTED_FREQUENCIES_MHZ = [1, 2, 4, 8, 16]

# Standard AVR Instruction Cycles Reference (AVR Instruction Set Manual)
# Core operations cycle lookup
AVR_OP_CYCLES = {
    "NOP": 1,
    "LDS_16bit": 4,      # LDS Rd, k (2 cycles on 8-bit, 4 for 16-bit pair)
    "STS_16bit": 4,      # STS k, Rr (2 cycles on 8-bit, 4 for 16-bit pair)
    "CP_CPC_16bit": 2,   # CP + CPC (1 + 1 cycle)
    "BRNE_taken": 2,     # Branch taken = 2 cycles, not taken = 1 cycle
    "BRNE_untaken": 1,
    "CLI": 1,
    "IN_SREG": 1,
    "OUT_SREG": 1,
    "RET": 4,
    "RETI": 4,
    "RCALL": 3,
    "ICALL": 3,
    "PUSH": 2,
    "POP": 2,
}

def cycles_to_time_us(cycles: float, freq_mhz: float) -> float:
    """Convert clock cycles to execution time in microseconds (µs)."""
    if freq_mhz <= 0:
        return 0.0
    return cycles / freq_mhz

def cycles_to_time_ns(cycles: float, freq_mhz: float) -> float:
    """Convert clock cycles to execution time in nanoseconds (ns)."""
    return cycles_to_time_us(cycles, freq_mhz) * 1000.0

def calculate_isr_cpu_load_pct(isr_cycles: float, tick_hz: float, freq_mhz: float) -> float:
    """
    Calculate the percentage of total CPU time consumed by the timer ISR.
    Formula: (ISR_Cycles * TICK_HZ / F_CPU_Hz) * 100%
    """
    f_cpu_hz = freq_mhz * 1_000_000.0
    if f_cpu_hz <= 0:
        return 0.0
    total_isr_cycles_per_sec = isr_cycles * tick_hz
    return (total_isr_cycles_per_sec / f_cpu_hz) * 100.0

def calculate_mcu_memory_utilization(mcu_name: str, ram_used_bytes: int, flash_used_bytes: int) -> dict:
    """Calculate RAM and Flash usage percentage for a specific MCU."""
    spec = MCU_SPECS.get(mcu_name)
    if not spec:
        raise ValueError(f"Unknown MCU: {mcu_name}")
    
    ram_pct = (ram_used_bytes / spec["ram_bytes"]) * 100.0
    flash_pct = (flash_used_bytes / spec["flash_bytes"]) * 100.0
    
    return {
        "mcu": mcu_name,
        "ram_used": ram_used_bytes,
        "ram_total": spec["ram_bytes"],
        "ram_pct": ram_pct,
        "flash_used": flash_used_bytes,
        "flash_total": spec["flash_bytes"],
        "flash_pct": flash_pct
    }
