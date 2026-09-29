song2notes.py - превращает MIDI / MP3 / WAV в одноголосную мелодию для пищалки
на Raspberry Pi Pico (MicroPython). Запускать на КОМПЬЮТЕРЕ.

## Установка:
    pip install pretty_midi            # для .mid / .midi
    pip install librosa numpy          # для .mp3 / .wav / .ogg / .flac (нужен ещё ffmpeg для mp3)

## Примеры:
    python song2notes.py song.mid --list                 # показать дорожки MIDI
    python song2notes.py song.mid                        # авто-выбор дорожки
    python song2notes.py song.mid --track 2 --bpm-scale 1.2
    python song2notes.py song.mp3 --start 30 --end 60    # 30 секунд с 0:30
    python song2notes.py song.mp3 --pin 15 -o main.py

Результат (по умолчанию <имя>_melody.py) загружается на Pico как main.py.

ВАЖНО: пищалка играет только ОДИН звук одновременно. Из MIDI берётся самая
верхняя нота аккорда (обычно это мелодия). Из MP3 определяется высота тона
одного голоса; с полной аранжировкой (гитара, бас, барабаны, вокал вместе)
результат будет шумным. Для MP3 лучше подавать вокал/соло отдельно или
использовать MIDI-версию песни.
