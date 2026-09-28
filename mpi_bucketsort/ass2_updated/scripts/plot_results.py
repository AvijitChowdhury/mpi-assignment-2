#!/usr/bin/env python3
"""
159.735 Assignment 2 - Generate Gustafson's Law performance graphs
NO external libraries needed - uses Python built-ins only.
Produces SVG files that open in any browser, and a text summary.

Usage:  python3 plot_results.py bucket_JOBID.out
Output: bucket_time_vs_np.svg
        bucket_speedup_vs_np.svg
        bucket_phases_vs_np.svg
        bucket_summary.txt
"""

import sys, re, os

# ── Parse output file ────────────────────────────────────────────────────────
def parse(filename):
    results, seq_time, cur = {}, None, None
    with open(filename) as f:
        for line in f:
            s = line.strip()
            m = re.match(r'==>\s*np\s*=\s*(\d+)', s)
            if m:
                cur = int(m.group(1)); results[cur] = {}

            if cur is None: continue

            for key, pat in [
                ('p1', r'Phase 1 \(scatter\)\s*:\s*([\d.]+)'),
                ('p2', r'Phase 2 \(small bkt\):\s*([\d.]+)'),
                ('p3', r'Phase 3 \(alltoallv\):\s*([\d.]+)'),
                ('p4', r'Phase 4 \(sort\)\s*:\s*([\d.]+)'),
                ('total', r'TOTAL\s*:\s*([\d.]+)'),
            ]:
                mm = re.match(pat, s)
                if mm: results[cur][key] = float(mm.group(1))

            ms = re.match(r'Elapsed time\s*:\s*([\d.]+)', s)
            if ms and seq_time is None and cur is None:
                seq_time = float(ms.group(1))

    results = {k: v for k, v in results.items() if 'total' in v}
    if not results: return seq_time, results
    if seq_time is None and 1 in results:
        seq_time = results[1].get('total')
    return seq_time, results

# ── SVG chart builder (same as gamma version) ─────────────────────────────────
def make_svg(title, xlabel, ylabel, series, filename, W=900, H=520, note=''):
    ML, MR, MT, MB = 80, 30, 60, 80
    PW = W - ML - MR; PH = H - MT - MB
    all_x = [v for s in series for v in s['x']]
    all_y = [v for s in series for v in s['y']]
    xmin, xmax = min(all_x), max(all_x)
    ymin, ymax = 0, max(all_y) * 1.18
    if ymin == ymax: ymax = ymin + 1

    def px(v): return ML + (v - xmin)/(xmax - xmin)*PW if xmax > xmin else ML+PW/2
    def py(v): return MT + PH - (v - ymin)/(ymax - ymin)*PH

    L = [f'<svg xmlns="http://www.w3.org/2000/svg" width="{W}" height="{H}">',
         f'<rect width="{W}" height="{H}" fill="white" stroke="#ccc"/>']

    for i in range(7):
        yv = ymin + i*(ymax-ymin)/6; yp = py(yv)
        L.append(f'<line x1="{ML}" y1="{yp:.1f}" x2="{ML+PW}" y2="{yp:.1f}" stroke="#e0e0e0" stroke-width="1"/>')
        L.append(f'<text x="{ML-8}" y="{yp+4:.1f}" text-anchor="end" font-size="11" fill="#555">{yv:.3f}</text>')

    for xv in sorted(set(all_x)):
        xp = px(xv)
        L.append(f'<line x1="{xp:.1f}" y1="{MT}" x2="{xp:.1f}" y2="{MT+PH}" stroke="#e8e8e8" stroke-width="1"/>')
        L.append(f'<text x="{xp:.1f}" y="{MT+PH+18}" text-anchor="middle" font-size="11" fill="#555">{int(xv) if xv==int(xv) else xv}</text>')

    L.append(f'<rect x="{ML}" y="{MT}" width="{PW}" height="{PH}" fill="none" stroke="#999" stroke-width="1.5"/>')

    for s in series:
        xs, ys = s['x'], s['y']
        pts = ' '.join(f'{px(x):.1f},{py(y):.1f}' for x,y in zip(xs,ys))
        dash = f'stroke-dasharray="{s["dash"]}"' if s.get('dash') else ''
        col  = s.get('color','#333')
        mk   = s.get('marker','circle')
        L.append(f'<polyline points="{pts}" fill="none" stroke="{col}" stroke-width="2.5" {dash} stroke-linejoin="round"/>')
        for x,y,yv in zip([px(xi) for xi in xs],[py(yi) for yi in ys],ys):
            if mk=='circle': L.append(f'<circle cx="{x:.1f}" cy="{y:.1f}" r="5" fill="white" stroke="{col}" stroke-width="2"/>')
            elif mk=='square': L.append(f'<rect x="{x-5:.1f}" y="{y-5:.1f}" width="10" height="10" fill="white" stroke="{col}" stroke-width="2"/>')
            else: L.append(f'<polygon points="{x:.1f},{y-6:.1f} {x-5:.1f},{y+4:.1f} {x+5:.1f},{y+4:.1f}" fill="white" stroke="{col}" stroke-width="2"/>')
            L.append(f'<text x="{x:.1f}" y="{y-10:.1f}" text-anchor="middle" font-size="10" fill="{col}">{yv:.3f}</text>')

    lx, ly = ML+10, MT+12
    for s in series:
        col = s.get('color','#333'); dash = f'stroke-dasharray="{s["dash"]}"' if s.get('dash') else ''
        L.append(f'<line x1="{lx}" y1="{ly}" x2="{lx+28}" y2="{ly}" stroke="{col}" stroke-width="2.5" {dash}/>')
        L.append(f'<text x="{lx+34}" y="{ly+4}" font-size="11" fill="#333">{s["label"]}</text>')
        ly += 20

    L.append(f'<text x="{W//2}" y="22" text-anchor="middle" font-size="14" font-weight="bold" fill="#222">{title}</text>')
    if note: L.append(f'<text x="{W//2}" y="40" text-anchor="middle" font-size="11" fill="#666">{note}</text>')
    L.append(f'<text x="{ML+PW//2}" y="{H-12}" text-anchor="middle" font-size="12" fill="#444">{xlabel}</text>')
    L.append(f'<text x="14" y="{MT+PH//2}" text-anchor="middle" font-size="12" fill="#444" transform="rotate(-90,14,{MT+PH//2})">{ylabel}</text>')
    L.append('</svg>')
    with open(filename,'w') as f: f.write('\n'.join(L))
    print(f"Saved: {filename}")

# ── Stacked bar SVG ───────────────────────────────────────────────────────────
def make_stacked_bar(title, np_vals, results, filename, W=920, H=540, note=''):
    ML,MR,MT,MB = 80,30,70,80; PW=W-ML-MR; PH=H-MT-MB
    phases = [('p1','Phase 1: Scatter','#e74c3c'),
              ('p2','Phase 2: Local Bucket','#f39c12'),
              ('p3','Phase 3: Alltoallv','#2ecc71'),
              ('p4','Phase 4: Sort','#3498db')]
    avail = [(k,lb,c) for k,lb,c in phases if any(k in results[p] for p in np_vals)]
    max_total = max(sum(results[p].get(k,0) for k,lb,c in avail) for p in np_vals)
    ymax = max_total * 1.2

    n = len(np_vals); bar_w = PW/(n*1.6); gap = PW/n

    def px(i): return ML + i*gap + gap/2
    def py(v): return MT + PH - v/ymax*PH

    L = [f'<svg xmlns="http://www.w3.org/2000/svg" width="{W}" height="{H}">',
         f'<rect width="{W}" height="{H}" fill="white" stroke="#ccc"/>']

    for i in range(7):
        yv = i*ymax/6; yp = py(yv)
        L.append(f'<line x1="{ML}" y1="{yp:.1f}" x2="{ML+PW}" y2="{yp:.1f}" stroke="#e0e0e0" stroke-width="1"/>')
        L.append(f'<text x="{ML-8}" y="{yp+4:.1f}" text-anchor="end" font-size="11" fill="#555">{yv:.3f}</text>')

    L.append(f'<rect x="{ML}" y="{MT}" width="{PW}" height="{PH}" fill="none" stroke="#999" stroke-width="1.5"/>')

    for i, p in enumerate(np_vals):
        bot = 0.0; cx = px(i)
        for key, lbl, col in avail:
            v = results[p].get(key, 0)
            if v <= 0: continue
            ybot = py(bot+v); hgt = py(bot)-py(bot+v)
            L.append(f'<rect x="{cx-bar_w/2:.1f}" y="{ybot:.1f}" width="{bar_w:.1f}" height="{hgt:.1f}" fill="{col}" opacity="0.85" stroke="white" stroke-width="0.5"/>')
            bot += v
        total = sum(results[p].get(k,0) for k,_,_ in avail)
        L.append(f'<text x="{cx:.1f}" y="{py(total)-6:.1f}" text-anchor="middle" font-size="10" fill="#333">{total:.3f}</text>')
        L.append(f'<text x="{cx:.1f}" y="{MT+PH+18}" text-anchor="middle" font-size="11" fill="#555">{p}</text>')

    lx,ly = ML+10, MT+12
    for _,lbl,col in avail:
        L.append(f'<rect x="{lx}" y="{ly-8}" width="16" height="12" fill="{col}" opacity="0.85"/>')
        L.append(f'<text x="{lx+22}" y="{ly+2}" font-size="11" fill="#333">{lbl}</text>')
        ly+=20

    L.append(f'<text x="{W//2}" y="22" text-anchor="middle" font-size="14" font-weight="bold" fill="#222">{title}</text>')
    if note: L.append(f'<text x="{W//2}" y="42" text-anchor="middle" font-size="11" fill="#666">{note}</text>')
    L.append(f'<text x="{ML+PW//2}" y="{H-12}" text-anchor="middle" font-size="12" fill="#444">Number of Processes (np)</text>')
    L.append(f'<text x="14" y="{MT+PH//2}" text-anchor="middle" font-size="12" fill="#444" transform="rotate(-90,14,{MT+PH//2})">Time (seconds)</text>')
    L.append('</svg>')
    with open(filename,'w') as f: f.write('\n'.join(L))
    print(f"Saved: {filename}")

def main():
    if len(sys.argv) < 2:
        files = sorted(f for f in os.listdir('.') if f.startswith('bucket_') and f.endswith('.out'))
        if not files: print("Usage: python3 plot_results.py bucket_JOBID.out"); return
        filename = files[-1]; print(f"Using: {filename}")
    else:
        filename = sys.argv[1]

    seq_time, results = parse(filename)
    if not results: print("No data found."); return

    np_vals = sorted(results); ELEM=8_000_000
    totals  = [results[p]['total'] for p in np_vals]
    T1      = results[1]['total'] if 1 in results else totals[0]

    sg_vals = []
    for p in np_vals:
        f = results[p].get('p1',0)/results[p]['total']
        sg_vals.append(p - f*(p-1))

    # ── Text summary ─────────────────────────────────────────────────────────
    lines = ["="*68, " 159.735 Assignment 2 - Gustafson's Law Summary", "="*68,
             f"{'np':>5} | {'N (total)':>13} | {'T_total':>9} | {'S_G(p)':>9} | {'Ideal':>7}", "-"*52]
    for p,t,sg in zip(np_vals, totals, sg_vals):
        lines.append(f"{p:>5} | {p*ELEM:>13,} | {t:>9.4f} | {sg:>9.4f} | {float(p):>7.1f}")
    lines.append("="*68)
    txt = '\n'.join(lines)
    print('\n'+txt+'\n')
    with open('bucket_summary.txt','w') as f: f.write(txt+'\n')
    print("Saved: bucket_summary.txt")

    # ── Plot 1: Time vs np ────────────────────────────────────────────────────
    make_svg(
        title='Assignment 2: Elapsed Time vs Number of Processes',
        note="Gustafson's Law - Elements per process = 8,000,000 (fixed)",
        xlabel='Number of Processes (np)', ylabel='Total Elapsed Time (seconds)',
        series=[
            {'label':'Measured T_total','x':np_vals,'y':totals,'color':'#1a6fba','marker':'circle'},
            {'label':f'np=1 baseline ({T1:.3f}s)','x':np_vals,'y':[T1]*len(np_vals),
             'color':'#aaa','dash':'8,4','marker':'square'},
        ],
        filename='bucket_time_vs_np.svg'
    )

    # ── Plot 2: Gustafson Scaled Speedup ─────────────────────────────────────
    make_svg(
        title="Assignment 2: Gustafson's Law Scaled Speedup vs Number of Processes",
        note="S_G(p) = p - f*(p-1),  f = T_scatter / T_total",
        xlabel='Number of Processes (np)', ylabel='Scaled Speedup S_G(p)',
        series=[
            {"label":"Measured S_G(p)",'x':np_vals,'y':sg_vals,'color':'#c0392b','marker':'square'},
            {'label':'Ideal S_G = p','x':np_vals,'y':[float(p) for p in np_vals],
             'color':'#aaa','dash':'8,4','marker':'circle'},
        ],
        filename='bucket_speedup_vs_np.svg'
    )

    # ── Plot 3: Phase Stacked Bar ─────────────────────────────────────────────
    make_stacked_bar(
        title='Assignment 2: Phase Breakdown vs Number of Processes',
        note="Gustafson's Law - N scales with np",
        np_vals=np_vals, results=results,
        filename='bucket_phases_vs_np.svg'
    )

    print("\nAll done! Open .svg files in any browser.")
    print("To copy to Windows:")
    print("  scp -P 2044 25016760@it096843.massey.ac.nz:~/MPI_version_6/mpi_bucketsort/*.svg .")

if __name__ == '__main__':
    main()
