import csv
import matplotlib.pyplot as plt
import re

with open('experiments/results.csv') as f:
    rows = list(csv.DictReader(f))

def parse(path):
    name = path.split('/')[-1]
    m = re.match(r'(\w+?)_(\d+[km])\.', name)
    return (m.group(1), m.group(2)) if m else (name, '?')

SIZE_BYTES = {'1k': 1024, '10k': 10240, '100k': 102400, '1m': 1048576, '5m': 5242880}

series = {}
for row in rows:
    typ, size = parse(row['file'])
    if size not in SIZE_BYTES:
        continue
    series.setdefault(typ, {})[size] = row

fig, axes = plt.subplots(1, 3, figsize=(18, 5))
order = ['1k', '10k', '100k', '1m', '5m']
x = [SIZE_BYTES[s] for s in order]

for typ, by_size in series.items():
    ratio   = [float(by_size[s]['ratio_percent']) / 100 if s in by_size else None for s in order]
    c_mean  = [float(by_size[s]['compress_mean_ms'])   if s in by_size else None for s in order]
    d_mean  = [float(by_size[s]['decompress_mean_ms']) if s in by_size else None for s in order]

    axes[0].plot(x, ratio, marker='o', label=typ)
    axes[1].plot(x, c_mean, marker='o', label=typ)
    axes[2].plot(x, d_mean, marker='o', label=typ)

for ax, title, ylabel in zip(
    axes,
    ['Степень сжатия', 'Время сжатия', 'Время разжатия'],
    ['Экономия, доля', 'Время, мс', 'Время, мс']
):
    ax.set_xscale('log')
    #ax.set_yscale('log')
    ax.set_xlabel('Размер файла, байт')
    ax.set_ylabel(ylabel)
    ax.set_title(title)
    ax.grid(True, which='both', alpha=0.3)
    ax.legend()

# только графики времени логарифм по Y
axes[1].set_yscale('log')
axes[2].set_yscale('log')

# для степени сжатия линейная шкала и горизонтальная линия на 0
axes[0].axhline(0, color='black', linewidth=0.8, linestyle='--', alpha=0.5)

plt.tight_layout()
plt.savefig('experiments/graphs.png', dpi=150)
print('Сохранено в experiments/graphs.png')