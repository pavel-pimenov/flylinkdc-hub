import socket
import threading
import time
import random
import string
import struct
import sys
from concurrent.futures import ThreadPoolExecutor, as_completed

class DCFuzzer:
    def __init__(self, host='127.0.0.1', port=411):
        self.host = host
        self.port = port
        self.timeout = 3
        self.max_workers = 8
        self.test_cases = []
        
    def generate_dc_commands(self):
        """Генерация DC++ команд для фаззинга"""
        print("Генерация DC++ тестовых случаев...")
        
        # Базовые DC++ команды
        base_commands = [
            # Стандартные команды
            "$MyNick ",
            "$Version ",
            "$GetNickList|",
            "$Lock ",
            "$Key ",
            "$Search ",
            "$SR ",
            "$ConnectToMe ",
            "$RevConnectToMe ",
            "$GetINFO ",
            "$Hello ",
            "$ValidateNick|",
            "$NickList ",
            "$SearchResult ",
            "$INFO ",
            "$Error ",
            
            # NMDC расширения
            "$Supports ",
            "$HubName ",
            "$HubTopic ",
            "$OpList ",
            "$To: ",
            "$From: ",
            "$Chat ",
            "$Quit ",
            "$LogedIn ",
            "$BadPass|",
            "$ZOn|",
            "$ZOff|"
        ]
        
        # Генерация фаззинг данных для каждой команды
        for cmd in base_commands:
            # 1. Нормальные значения
            if cmd.endswith(' '):
                normal_values = [
                    cmd + "TestUser|",
                    cmd + "VeryLongNickname123456|",
                    cmd + "UserWith Spaces|",
                    cmd + "UserWith$Special|Chars|",
                    cmd + "UserWith\\Backslashes|",
                    cmd + "UserWith/Forwardslashes|"
                ]
                self.test_cases.extend([v.encode('utf-8') for v in normal_values])
            
            # 2. Случайные строки для параметров
            for _ in range(5):
                if cmd.endswith(' '):
                    random_param = ''.join(random.choices(
                        string.ascii_letters + string.digits + string.punctuation + ' ',
                        k=random.randint(1, 100)
                    )) + '|'
                    self.test_cases.append((cmd + random_param).encode('utf-8'))
        
        # 3. Форматные строки и инъекции
        format_strings = [
            b"$MyNick %s" * 10 + b"|",
            b"$MyNick %n%n%n%n%n|",
            b"$MyNick A" * 1000 + b"|",
            b"$Search " + b"A" * 500 + b"|",
            b"$Lock " + b"\x00" * 50 + b"|",
            b"$Key " + b"\xFF" * 50 + b"|",
        ]
        self.test_cases.extend(format_strings)
        
        # 4. Неполные команды
        incomplete_commands = [
            b"$MyNick",
            b"$Version",
            b"$Lock",
            b"$Key",
            b"$Search",
            b"$",
            b"|",
            b"",
            b"\x00",
            b"\xFF"
        ]
        self.test_cases.extend(incomplete_commands)
        
        # 5. Бинарные данные в командах
        binary_commands = [
            b"$MyNick " + bytes([random.randint(0, 255) for _ in range(50)]) + b"|",
            b"$Lock " + b"\x00\x01\x02\x03\x04\x05" * 10 + b"|",
            b"$Key " + b"\xFF\xFE\xFD\xFC" * 10 + b"|",
        ]
        self.test_cases.extend(binary_commands)
        
        # 6. Очень длинные команды
        long_commands = [
            b"$MyNick " + b"A" * 5000 + b"|",
            b"$Search " + b"B" * 5000 + b"|",
            b"$Lock " + b"C" * 5000 + b"|",
        ]
        self.test_cases.extend(long_commands)
        
        # 7. Команды с неправильным форматом
        malformed_commands = [
            b"$MyNick TestUser",  # без |
            b"$Version 1.0",     # без |
            b"$GetNickList",     # без |
            b"MyNick TestUser|", # без $
            b"$InvalidCommand Test|",
            b"$$DoubleDollar Test|",
            b"$MyNick |Test|User|",
            b"$MyNick Test\\|User|",
            b"$MyNick Test/User|",
        ]
        self.test_cases.extend(malformed_commands)
        
        print(f"Сгенерировано {len(self.test_cases)} DC++ тестовых случаев")
    
    def send_dc_command(self, test_data):
        """Отправка DC++ команды и анализ ответа"""
        try:
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.settimeout(self.timeout)
            
            sock.connect((self.host, self.port))
            
            # Получаем приветствие
            hello = sock.recv(1024)
            
            # Отправка тестовых данных
            sock.send(test_data)
            
            # Попытка получить ответ
            try:
                response = sock.recv(4096)
                return {
                    'command': test_data,
                    'response': response,
                    'hello': hello,
                    'error': None,
                    'status': 'success'
                }
            except socket.timeout:
                return {
                    'command': test_data,
                    'response': None,
                    'hello': hello,
                    'error': 'timeout',
                    'status': 'timeout'
                }
            except Exception as e:
                return {
                    'command': test_data,
                    'response': None,
                    'hello': hello,
                    'error': str(e),
                    'status': 'error'
                }
            finally:
                sock.close()
                
        except Exception as e:
            return {
                'command': test_data,
                'response': None,
                'hello': None,
                'error': str(e),
                'status': 'connection_error'
            }
    
    def analyze_response(self, result):
        """Анализ ответа сервера"""
        if result['response']:
            try:
                response_str = result['response'].decode('utf-8', errors='ignore')
                if '$Error' in response_str:
                    return 'error_response'
                elif any(cmd in response_str for cmd in ['$Hello', '$Key', '$ValidateNick']):
                    return 'valid_response'
                else:
                    return 'unknown_response'
            except:
                return 'binary_response'
        return 'no_response'
    
    def run_fuzzing(self):
        """Запуск DC++ фаззинга"""
        print(f"Запуск DC++ фаззинга на {self.host}:{self.port}")
        print(f"Количество тестов: {len(self.test_cases)}")
        print(f"Потоков: {self.max_workers}")
        print("-" * 60)
        
        results = {
            'success': 0,
            'timeout': 0,
            'error': 0,
            'connection_error': 0,
            'crashes': 0,
            'error_responses': 0,
            'no_responses': 0,
            'valid_responses': 0
        }
        
        interesting_cases = []
        start_time = time.time()
        
        with ThreadPoolExecutor(max_workers=self.max_workers) as executor:
            future_to_test = {
                executor.submit(self.send_dc_command, test_case): test_case 
                for test_case in self.test_cases
            }
            
            for i, future in enumerate(as_completed(future_to_test)):
                result = future.result()
                results[result['status']] += 1
                
                # Анализ ответа
                if result['status'] == 'success':
                    response_type = self.analyze_response(result)
                    results[response_type + 's'] += 1
                
                # Прогресс
                if (i + 1) % 50 == 0:
                    print(f"Обработано: {i + 1}/{len(self.test_cases)}")
                
                # Логирование интересных случаев
                if result['status'] in ['error', 'connection_error']:
#                    print(f"\n! Потенциальная проблема: {result['command'][:100]}...")
#                    print(f"   Ошибка: {result['error']}")
                    results['crashes'] += 1
                    interesting_cases.append(result)
                
                elif result['status'] == 'timeout':
                    interesting_cases.append(result)
        
        end_time = time.time()
        
        # Вывод результатов
        print("\n" + "=" * 60)
        print("РЕЗУЛЬТАТЫ DC++ ФАЗЗИНГА:")
        print("=" * 60)
        print(f"Успешных запросов: {results['success']}")
        print(f"  - Валидных ответов: {results['valid_responses']}")
        print(f"  - Ответов с ошибкой: {results['error_responses']}")
        print(f"Таймаутов: {results['timeout']}")
        print(f"Ошибок: {results['error']}")
        print(f"Ошибок подключения: {results['connection_error']}")
        print(f"Потенциальных падений: {results['crashes']}")
        print(f"Общее время: {end_time - start_time:.2f} секунд")
        print(f"Скорость: {len(self.test_cases) / (end_time - start_time):.2f} запросов/секунду")
        
        # Вывод интересных случаев
        if interesting_cases:
            print(f"\nНайдено {len(interesting_cases)} интересных случаев:")
            for case in interesting_cases[:5]:  # Первые 5
                print(f"  Команда: {case['command'][:50]}...")
                print(f"  Статус: {case['status']}, Ошибка: {case['error']}")
        
        return results

def main():
    if len(sys.argv) != 3:
        print("Использование: python dc_fuzzer.py <host> <port>")
        print("Пример: python dc_fuzzer.py 127.0.0.1 411")
        sys.exit(1)
    
    host = sys.argv[1]
    port = int(sys.argv[2])
    
    fuzzer = DCFuzzer(host, port)
    fuzzer.generate_dc_commands()
    fuzzer.run_fuzzing()

if __name__ == "__main__":
    main()
