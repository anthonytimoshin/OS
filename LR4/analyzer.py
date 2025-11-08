import glob
import math
import statistics

def compute_confidence_interval(data, confidence=0.95):
    n = len(data)
    mean = statistics.mean(data)
    stdev = statistics.stdev(data)
    sem = stdev / math.sqrt(n)

    if n <= 10:
        t = 2.262
    elif n <= 20:
        t = 2.093
    elif n <= 30:
        t = 2.045
    else:
        t = 1.984

    margin = t * sem
    return mean, mean - margin, mean + margin

# === ИСПРАВЛЕНО: сортировка по числу потоков ===
files = sorted(glob.glob("times_*.txt"), key=lambda x: int(x.split('_')[1].split('.')[0]))

print("Анализ времени выполнения (95% доверительный интервал)\n")
print(f"{'Потоки':<8} {'Среднее':<10} {'95% CI':<25}")
print("-" * 50)

for file in files:
    n_threads = file.split('_')[1].split('.')[0]
    with open(file, 'r') as f:
        times = [float(line.strip()) for line in f if line.strip()]
    
    mean, ci_low, ci_high = compute_confidence_interval(times)
    print(f"{n_threads:<8} {mean:<10.4f} [{ci_low:.4f}, {ci_high:.4f}]")
  
