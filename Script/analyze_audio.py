'''
Use librosa to transform music to time-frequency related data.
The output csv file is used for `FVmdLightControlTableRow` in Unreal.
'''
import argparse
import os

import librosa
import numpy as np
import pandas as pd


# ============================================================
# Configuration
# ============================================================

# Sample rate while read in sound file
SR = 44100


# The number of samples between successive frames
# 1024 / 44100 ≈ 23.22 ms ≈ 43 FPS
#
# 512  -> ≈ 86 FPS
# 1024 -> ≈ 43 FPS
# 2048 -> ≈ 21.5 FPS
HOP_LENGTH = 1024

# FFT Size
N_FFT = 4096

EPSILON = 1e-12


# ============================================================
# Normalization
# ============================================================

def percentile_normalize(
    x,
    low_percentile=2.0,
    high_percentile=98.0
):
    """
    Robust normalization -> [0, 1]
    Generate *_norm datas
    """

    x = np.asarray(
        x,
        dtype=np.float64
    )

    low = np.percentile(
        x,
        low_percentile
    )

    high = np.percentile(
        x,
        high_percentile
    )

    if high - low < EPSILON:
        return np.zeros_like(x)

    result = (
        x - low
    ) / (
        high - low
    )

    return np.clip(
        result,
        0.0,
        1.0
    )


def smooth(
    x,
    attack=0.35,
    release=0.12
):
    """
    Attack / Release smoothing。
    For normalized feature。
    Energy data will not be changed.
    """

    x = np.asarray(
        x,
        dtype=np.float64
    )

    result = np.zeros_like(x)

    if len(x) == 0:
        return result

    result[0] = x[0]

    for i in range(1, len(x)):

        if x[i] > result[i - 1]:
            coefficient = attack
        else:
            coefficient = release

        result[i] = (
            result[i - 1]
            + (
                x[i]
                - result[i - 1]
            ) * coefficient
        )

    return result


# ============================================================
# Frequency Band Energy
# ============================================================

def band_power(
    power_spectrum,
    freqs,
    low,
    high
):
    """
    power_spectrum:
        |STFT|²
    """

    mask = (
        (freqs >= low)
        &
        (freqs < high)
    )

    if not np.any(mask):

        return np.zeros(
            power_spectrum.shape[1],
            dtype=np.float64
        )

    return np.mean(
        power_spectrum[mask],
        axis=0
    )


# ============================================================
# Main Analysis
# ============================================================

def analyze_music(
    input_file,
    output_file
):

    print(
        f"Loading: {input_file}"
    )

    # ========================================================
    # Load Audio
    # ========================================================

    y, sr = librosa.load(
        input_file,
        sr=SR,
        mono=True
    )

    duration = len(y) / sr

    print(
        f"Sample Rate : {sr}"
    )

    print(
        f"Duration    : {duration:.2f} sec"
    )

    # ========================================================
    # STFT
    # ========================================================

    stft = librosa.stft(
        y,
        n_fft=N_FFT,
        hop_length=HOP_LENGTH,
        window="hann"
    )

    magnitude = np.abs(
        stft
    )

    # Power spectrum
    power = (
        magnitude ** 2
    )

    freqs = librosa.fft_frequencies(
        sr=sr,
        n_fft=N_FFT
    )

    frame_count = power.shape[1]

    # ========================================================
    # Time
    # ========================================================

    times = librosa.frames_to_time(
        np.arange(frame_count),
        sr=sr,
        hop_length=HOP_LENGTH
    )

    # ========================================================
    # 1. RMS Energy
    # ========================================================

    rms_energy = librosa.feature.rms(
        S=magnitude,
        frame_length=N_FFT,
        hop_length=HOP_LENGTH
    )[0]

    # RMS normalized
    rms_norm = percentile_normalize(
        rms_energy
    )

    rms_norm = smooth(
        rms_norm
    )

    # ========================================================
    # 2. Frequency Band Energy
    # ========================================================
    #
    # Bass:
    #       20 - 150 Hz
    #
    # Low Mid:
    #       150 - 500 Hz
    #
    # Mid:
    #       500 - 2000 Hz
    #
    # High Mid:
    #       2000 - 6000 Hz
    #
    # Treble:
    #       6000 - 16000 Hz
    #
    # ========================================================

    bass_energy = band_power(
        power,
        freqs,
        20,
        150
    )

    low_mid_energy = band_power(
        power,
        freqs,
        150,
        500
    )

    mid_energy = band_power(
        power,
        freqs,
        500,
        2000
    )

    high_mid_energy = band_power(
        power,
        freqs,
        2000,
        6000
    )

    treble_energy = band_power(
        power,
        freqs,
        6000,
        16000
    )

    # ========================================================
    # 3. Normalized Frequency Features
    # ========================================================

    bass_norm = percentile_normalize(
        bass_energy
    )

    low_mid_norm = percentile_normalize(
        low_mid_energy
    )

    mid_norm = percentile_normalize(
        mid_energy
    )

    high_mid_norm = percentile_normalize(
        high_mid_energy
    )

    treble_norm = percentile_normalize(
        treble_energy
    )

    # Smooth normalized features
    bass_norm = smooth(bass_norm)
    low_mid_norm = smooth(low_mid_norm)
    mid_norm = smooth(mid_norm)
    high_mid_norm = smooth(high_mid_norm)
    treble_norm = smooth(treble_norm)

    # ========================================================
    # 4. Total Spectral Energy
    # ========================================================

    total_spectral_energy = np.sum(
        power,
        axis=0
    )

    # ========================================================
    # 5. Relative Frequency Ratios
    # ========================================================
    #
    #     Bass / Treble Color
    #     Low / High Ratio
    #
    # ========================================================

    bass_ratio = (
        bass_energy
        /
        (
            total_spectral_energy
            + EPSILON
        )
    )

    low_mid_ratio = (
        low_mid_energy
        /
        (
            total_spectral_energy
            + EPSILON
        )
    )

    mid_ratio = (
        mid_energy
        /
        (
            total_spectral_energy
            + EPSILON
        )
    )

    high_mid_ratio = (
        high_mid_energy
        /
        (
            total_spectral_energy
            + EPSILON
        )
    )

    treble_ratio = (
        treble_energy
        /
        (
            total_spectral_energy
            + EPSILON
        )
    )

    # ========================================================
    # 6. Spectral Centroid
    # ========================================================

    spectral_centroid = (
        librosa.feature.spectral_centroid(
            S=magnitude,
            sr=sr
        )[0]
    )

    spectral_centroid_norm = np.clip(
        (
            spectral_centroid
            - 200.0
        )
        /
        (
            8000.0
            - 200.0
        ),
        0.0,
        1.0
    )

    spectral_centroid_norm = smooth(
        spectral_centroid_norm
    )

    # ========================================================
    # 7. Spectral Bandwidth
    # ========================================================

    spectral_bandwidth = (
        librosa.feature.spectral_bandwidth(
            S=magnitude,
            sr=sr
        )[0]
    )

    spectral_bandwidth_norm = (
        percentile_normalize(
            spectral_bandwidth
        )
    )

    spectral_bandwidth_norm = smooth(
        spectral_bandwidth_norm
    )

    # ========================================================
    # 8. Spectral Rolloff
    # ========================================================

    spectral_rolloff = (
        librosa.feature.spectral_rolloff(
            S=magnitude,
            sr=sr,
            roll_percent=0.85
        )[0]
    )

    spectral_rolloff_norm = np.clip(
        (
            spectral_rolloff
            - 500.0
        )
        /
        (
            12000.0
            - 500.0
        ),
        0.0,
        1.0
    )

    spectral_rolloff_norm = smooth(
        spectral_rolloff_norm
    )

    # ========================================================
    # 9. Onset
    # ========================================================

    onset = librosa.onset.onset_strength(
        y=y,
        sr=sr,
        hop_length=HOP_LENGTH
    )

    onset_norm = percentile_normalize(
        onset
    )

    onset_norm = smooth(
        onset_norm,
        attack=0.8,
        release=0.05
    )

    # ========================================================
    # 10. Spectral Flux
    # ========================================================

    normalized_spectrum = (
        power
        /
        (
            np.sum(
                power,
                axis=0,
                keepdims=True
            )
            + EPSILON
        )
    )

    spectral_flux = np.zeros(
        frame_count,
        dtype=np.float64
    )

    for i in range(1, frame_count):

        difference = (
            normalized_spectrum[:, i]
            - normalized_spectrum[:, i - 1]
        )

        # Keep positive change
        difference = np.maximum(
            difference,
            0.0
        )

        spectral_flux[i] = np.sum(
            difference
        )

    spectral_flux_norm = (
        percentile_normalize(
            spectral_flux
        )
    )

    spectral_flux_norm = smooth(
        spectral_flux_norm,
        attack=0.8,
        release=0.1
    )

    # ========================================================
    # 11. Beat Tracking
    # ========================================================

    tempo, beat_frames = (
        librosa.beat.beat_track(
            y=y,
            sr=sr,
            hop_length=HOP_LENGTH
        )
    )

    if np.ndim(tempo) > 0:

        bpm = float(
            np.asarray(
                tempo
            ).flatten()[0]
        )

    else:

        bpm = float(
            tempo
        )

    beat = np.zeros(
        frame_count,
        dtype=np.float64
    )

    for frame in beat_frames:

        if frame < frame_count:

            beat[frame] = 1.0

    # ========================================================
    # 12. Create DataFrame
    # ========================================================

    df = pd.DataFrame({

        # ====================================================
        # Time
        # ====================================================

        "time":
            times,

        # ====================================================
        # RMS
        # ====================================================

        "rms_energy":
            rms_energy,

        "rms_norm":
            rms_norm,

        # ====================================================
        # Raw Frequency Energy
        # ====================================================

        "bass_energy":
            bass_energy,

        "low_mid_energy":
            low_mid_energy,

        "mid_energy":
            mid_energy,

        "high_mid_energy":
            high_mid_energy,

        "treble_energy":
            treble_energy,

        # ====================================================
        # Normalized Frequency Energy
        # ====================================================

        "bass_norm":
            bass_norm,

        "low_mid_norm":
            low_mid_norm,

        "mid_norm":
            mid_norm,

        "high_mid_norm":
            high_mid_norm,

        "treble_norm":
            treble_norm,

        # ====================================================
        # Relative Frequency Ratios
        # ====================================================

        "bass_ratio":
            bass_ratio,

        "low_mid_ratio":
            low_mid_ratio,

        "mid_ratio":
            mid_ratio,

        "high_mid_ratio":
            high_mid_ratio,

        "treble_ratio":
            treble_ratio,

        # ====================================================
        # Spectral Features
        # ====================================================

        "spectral_centroid":
            spectral_centroid,

        "spectral_centroid_norm":
            spectral_centroid_norm,

        "spectral_bandwidth":
            spectral_bandwidth,

        "spectral_bandwidth_norm":
            spectral_bandwidth_norm,

        "spectral_rolloff":
            spectral_rolloff,

        "spectral_rolloff_norm":
            spectral_rolloff_norm,

        # ====================================================
        # Temporal Features
        # ====================================================

        "onset":
            onset,

        "onset_norm":
            onset_norm,

        "spectral_flux":
            spectral_flux,

        "spectral_flux_norm":
            spectral_flux_norm,

        "beat":
            beat,

        # ====================================================
        # Global
        # ====================================================

        "bpm":
            np.full(
                frame_count,
                bpm,
                dtype=np.float64
            )
    })

    # ========================================================
    # Save
    # ========================================================
    df.insert(
            0,
            "index",
            np.arange(len(df), dtype=np.int64)
        )
    
    df.to_csv(
        output_file,
        index=False,
        float_format="%.6f"
    )

    # ========================================================
    # Statistics
    # ========================================================

    print()
    print(
        "=============================================="
    )

    print(
        "Analysis Complete"
    )

    print(
        "=============================================="
    )

    print(
        f"Output      : {output_file}"
    )

    print(
        f"Frames      : {frame_count}"
    )

    print(
        f"FPS         : {sr / HOP_LENGTH:.2f}"
    )

    print(
        f"Duration    : {duration:.2f} sec"
    )

    print(
        f"BPM         : {bpm:.2f}"
    )

    print(
        f"Beat Count  : {len(beat_frames)}"
    )

    print(
        "=============================================="
    )

# ============================================================
# Command Line
# ============================================================

if __name__ == "__main__":

    parser = argparse.ArgumentParser(
        description=(
            "Analyze music and export "
            "audio features to CSV."
        )
    )

    parser.add_argument("-i", "--input",help="Input audio file")
    parser.add_argument("-o", "--output", help="Output CSV file", default="")
    args = parser.parse_args()

    file_output = args.output
    if not file_output:
        if not args.input:
            raise SystemExit("Input audio file is required.")

        file_output = os.path.splitext(args.input)[0] + ".csv"
        print("Output to input folder: {0}".format(file_output))

    analyze_music(args.input, file_output)
