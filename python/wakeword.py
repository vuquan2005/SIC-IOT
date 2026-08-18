#!/home/pi/dev/venv/bin/python
import sys
import os
import io
import time
import base64
import subprocess
import requests
import warnings
warnings.filterwarnings("ignore") # Ẩn cảnh báo không cần thiết

import numpy as np
import sounddevice as sd
import soundfile as sf
import openwakeword
from openwakeword.model import Model
from gtts import gTTS

# ================= CẤU HÌNH HỆ THỐNG =================
GEMINI_API_KEY = ""
OPENHAB_URL = "http://localhost:8080"
OPENHAB_API_TOKEN = ""
AUDIO_OUTPUT_PATH = "/dev/shm/response.mp3"

# ================= CẤU HÌNH MICRO INMP441 & WAKEWORD =================
DEVICE_MIC = 0             # Index Micro INMP441
MIC_SAMPLE_RATE = 48000    # Tần số lấy mẫu gốc I2S của INMP441 (48kHz)
CHUNK_SIZE = 3840          # 3840 samples cho 80ms tại 48kHz (tương đương 1280 samples tại 16kHz)
THRESHOLD = 0.5            # Ngưỡng tin cậy kích hoạt (0.5 = 50%)
COMMAND_DURATION = 5       # Thời gian thu âm câu lệnh (giây)
WAKEWORD_MODELS = ["alexa", "hey_jarvis"] # Từ khóa mặc định có sẵn
# ======================================================

def print_visual_box(title, content, color_code="36"):
    """In khung giao diện màu sắc trong Terminal"""
    lines = content.split('\n')
    width = max(len(line) for line in lines) + 4
    width = max(width, len(title) + 6, 60)
    
    border_top = f"\033[{color_code}m┌{'─' * (width - 2)}┐\033[0m"
    border_bottom = f"\033[{color_code}m└{'─' * (width - 2)}┘\033[0m"
    
    print(border_top)
    print(f"\033[{color_code}m│\033[1;33m  {title:<{width - 5}}\033[0;{color_code}m│\033[0m")
    print(f"\033[{color_code}m├{'─' * (width - 2)}┤\033[0m")
    for line in lines:
        print(f"\033[{color_code}m│\033[1;37m  {line:<{width - 5}}\033[0;{color_code}m│\033[0m")
    print(border_bottom)

def process_command_audio(recording):
    """Xử lý mảng âm thanh câu lệnh đã thu từ Stream"""
    ch0_peak = np.max(np.abs(recording[:, 0]))
    ch1_peak = np.max(np.abs(recording[:, 1])) if recording.shape[1] > 1 else 0
    selected_ch = recording[:, 1] if ch1_peak > ch0_peak else recording[:, 0]
    peak_val = max(ch0_peak, ch1_peak)

    if peak_val > 0:
        scale_factor = min((32767 * 0.65) / peak_val, 4.0)
        boosted = (selected_ch.astype(np.float32) * scale_factor).clip(-32768, 32767).astype(np.int16)
    else:
        boosted = selected_ch

    # Resample 48kHz -> 16kHz
    downsampled = boosted[::3]

    wav_buffer = io.BytesIO()
    sf.write(wav_buffer, downsampled, 16000, format='WAV', subtype='PCM_16')
    wav_buffer.seek(0)
    return wav_buffer.read()

def send_audio_to_gemini(wav_bytes):
    """Gửi dữ liệu âm thanh tới Gemini API để nhận dạng giọng nói (STT)"""
    print("[🚀 GEMINI STT]: Đang gửi giọng nói tới Gemini AI...")
    start_time = time.time()
    try:
        audio_b64 = base64.b64encode(wav_bytes).decode('utf-8')
        gemini_url = f"https://generativelanguage.googleapis.com/v1beta/models/gemini-3.5-flash-lite:generateContent?key={GEMINI_API_KEY}"
        
        payload = {
            "contents": [
                {
                    "role": "user",
                    "parts": [
                        {"text": "Hãy nghe âm thanh này và trích xuất chính xác câu lệnh bằng tiếng Việt của người dùng (ví dụ: 'bật đèn', 'tắt đèn', 'nhiệt độ bao nhiêu', 'thời tiết thế nào'). Chỉ trả về câu lệnh văn bản ngắn gọn, không giải thích thêm."},
                        {"inlineData": {"mimeType": "audio/wav", "data": audio_b64}}
                    ]
                }
            ]
        }

        resp = requests.post(gemini_url, json=payload, timeout=20)
        if resp.status_code == 200:
            res_json = resp.json()
            transcript = res_json['candidates'][0]['content']['parts'][0]['text'].strip()
            elapsed = time.time() - start_time
            display_info = f"{transcript}\n\n⏱️  Thời gian nhận dạng STT: {elapsed:.2f} giây"
            print_visual_box("🎙️ VĂN BẢN NHẬN DẠNG TỪ GIỌNG NÓI (STT)", display_info, "35")
            return transcript
        else:
            print(f"[❌ LỖI GEMINI STT {resp.status_code}]: {resp.text}")
            return None
    except Exception as e:
        print(f"[❌ LỖI KẾT NỐI GEMINI STT]: {e}")
        return None

def send_to_openhab_gemini(prompt_text):
    """Gửi câu lệnh lên openHAB Gemini HLI để điều khiển thiết bị"""
    headers = {
        "Authorization": f"Bearer {OPENHAB_API_TOKEN}",
        "Content-Type": "text/plain; charset=utf-8",
        "Accept": "text/plain"
    }

    start_time = time.time()
    try:
        response = requests.post(
            f"{OPENHAB_URL}/rest/voice/interpreters/gemini",
            headers=headers,
            data=prompt_text.encode('utf-8'),
            timeout=25
        )

        if response.status_code == 200:
            response.encoding = 'utf-8'
            reply_text = response.text.strip()
            elapsed = time.time() - start_time
            display_msg = f"{reply_text}\n\n⏱️  Thời gian xử lý openHAB: {elapsed:.2f} giây"
            print_visual_box("🤖 GEMINI HLI PHẢN HỒI (TEXT OUTPUT)", display_msg, "32")
            return reply_text
        else:
            print(f"[❌ LỖI OPENHAB {response.status_code}]: {response.text}")
            return None
    except Exception as e:
        print(f"[❌ LỖI MẠNG OPENHAB]: {e}")
        return None

def play_tts_audio(text):
    """Phát phản hồi bằng gTTS ra loa"""
    print("\n[⏳ TTS SYSTEM]: Đang tạo giọng nói...")
    try:
        tts = gTTS(text=text, lang='vi')
        tts.save(AUDIO_OUTPUT_PATH)
        # Chèn 500ms khoảng lặng ở đầu câu (-af adelay=500|500) để đánh thức loa Bluetooth trước khi phát tiếng
        cmd = ["ffplay", "-nodisp", "-autoexit", "-loglevel", "quiet", "-af", "adelay=500|500", AUDIO_OUTPUT_PATH]
        print("[🔊 SPEAKER]: Đang phát câu trả lời ra loa...")
        subprocess.run(cmd, check=True)
        print("[🎉 HOÀN THÀNH]: Đã phát xong câu trả lời!\n")
    except Exception as e:
        print(f"[❌ LỖI PHÁT ÂM THANH]: {e}")
    finally:
        if os.path.exists(AUDIO_OUTPUT_PATH):
            os.remove(AUDIO_OUTPUT_PATH)

def main():
    print("\n" + "=" * 65)
    print(" 🔊 TRỢ LÝ GIỌNG NÓI KÍCH HOẠT TỪ KHÓA OFFLINE (OPENWAKEWORD)")
    print("=" * 65)
    print("⏳ Đang tải mô hình từ khóa openWakeWord...")
    
    try:
        oww_model = Model(
            wakeword_models=WAKEWORD_MODELS,
            inference_framework="onnx"
        )
        print(f"✅ [SẴN SÀNG]: Mô hình đã tải xong ({', '.join(WAKEWORD_MODELS)})")
    except Exception as e:
        print(f"[⚠️ DOWNLOADING MODELS]: Đang tải mô hình từ khóa...")
        openwakeword.utils.download_models()
        oww_model = Model(
            wakeword_models=WAKEWORD_MODELS,
            inference_framework="onnx"
        )
        print("✅ [SẴN SÀNG]: Đã tải xong mô hình!")

    print("\n👉 Hãy gọi từ khóa: \"Alexa\" hoặc \"Hey Jarvis\" để kích hoạt trợ lý!\n")

    # Các biến trạng thái luồng thu âm
    state = "WAKEWORD" # "WAKEWORD" hoặc "RECORDING"
    command_buffer = []
    frames_needed = int((COMMAND_DURATION * MIC_SAMPLE_RATE) / CHUNK_SIZE)

    def audio_callback(indata, frames, time_info, status):
        nonlocal state, command_buffer
        
        # Chọn channel tốt hơn
        ch0_max = np.max(np.abs(indata[:, 0]))
        ch1_max = np.max(np.abs(indata[:, 1])) if indata.shape[1] > 1 else 0
        raw_ch = indata[:, 1] if ch1_max > ch0_max else indata[:, 0]
        
        if state == "WAKEWORD":
            # Hạ mẫu từ 48kHz -> 16kHz cho openWakeWord (3840 -> 1280 samples)
            pcm_16k = raw_ch[::3]
            prediction = oww_model.predict(pcm_16k)
            
            for model_name, score in oww_model.prediction_buffer.items():
                if score[-1] >= THRESHOLD:
                    print(f"\n🎉 [ĐÃ PHÁT HIỆN TỪ KHÓA WAKEWORD!]: Từ khóa \"{model_name.upper()}\" (Độ tin cậy: {score[-1]*100:.1f}%)")
                    print(f"[🔴 ĐANG THU ÂM CÂU LỆNH]: Hãy nói lệnh của bạn trong {COMMAND_DURATION} giây...")
                    oww_model.reset()
                    state = "RECORDING"
                    command_buffer = []
                    break

        elif state == "RECORDING":
            command_buffer.append(indata.copy())
            remaining_chunks = frames_needed - len(command_buffer)
            remaining_sec = max(0, int((remaining_chunks * CHUNK_SIZE) / MIC_SAMPLE_RATE))
            print(f"   ⏱️  Còn lại: {remaining_sec} giây...", end="\r")
            
            if len(command_buffer) >= frames_needed:
                print("\n[⏹️ DỪNG THU]: Đang xử lý câu lệnh...")
                full_recording = np.concatenate(command_buffer, axis=0)
                state = "PROCESSING"

                # Xử lý âm thanh thu được
                wav_bytes = process_command_audio(full_recording)
                if wav_bytes:
                    transcript = send_audio_to_gemini(wav_bytes)
                    if transcript and "không nghe" not in transcript.lower():
                        reply_text = send_to_openhab_gemini(transcript)
                        if reply_text:
                            play_tts_audio(reply_text)
                
                print("👉 [LẮNG NGHE]: Đang tiếp tục lắng nghe từ khóa \"Alexa\" / \"Hey Jarvis\"...\n")
                command_buffer = []
                oww_model.reset()
                state = "WAKEWORD"

    try:
        with sd.InputStream(
            samplerate=MIC_SAMPLE_RATE,  # 48000 Hz chuẩn phần cứng INMP441
            channels=2,
            blocksize=CHUNK_SIZE,       # 3840 samples ~ 80ms
            device=DEVICE_MIC,
            dtype='int16',
            callback=audio_callback
        ):
            while True:
                time.sleep(0.1)
    except KeyboardInterrupt:
        print("\nĐã thoát chương trình openWakeWord.")
    except Exception as e:
        print(f"[❌ LỖI AUDIO STREAM]: {e}")

if __name__ == "__main__":
    main()
