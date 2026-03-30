#!/usr/bin/env python3
"""
Convert a PolyBee evolutionary run log file into a .cfg file that represents
the best individual found during the run.

The output config has:
  - All scalar parameters from the log header
  - evolve=false, visualise=true, bee-path-record-len=100
  - Hive / tunnel-entrance values from the header (or from the best individual
    if those items were being evolved)
  - Barrier values from the best individual (if barriers were evolved),
    otherwise from the header
  - Base patch values from the header, plus one patch= entry per evolved
    bridge (if bridges were evolved)

Usage:
    ./best_individual_to_cfg.py <logfile> [-o <output.cfg>]
"""

import argparse
import re
import sys
from pathlib import Path


# ---------------------------------------------------------------------------
# Argument parsing
# ---------------------------------------------------------------------------

def parse_args():
    parser = argparse.ArgumentParser(
        description='Convert PolyBee evolutionary run log to best-individual config file')
    parser.add_argument('logfile', help='Input .txt log file from an evolutionary run')
    parser.add_argument('-o', '--output',
                        help='Output .cfg file path (default: same directory as input, '
                             'with "out-" prefix replaced by "best-" and .cfg extension)')
    return parser.parse_args()


# ---------------------------------------------------------------------------
# evolve-spec parsing
# ---------------------------------------------------------------------------

def parse_evolve_spec(spec_str):
    """
    Parse an evolve-spec string such as 'B:9,30;X:9,50' and return a dict
    describing what was being evolved.
    """
    result = {
        'evolve_entrances':   False, 'num_entrances':    0, 'entrance_width': 0.0,
        'evolve_hives':       False,
        'num_hives_inside':   0, 'num_hives_outside': 0, 'num_hives_free': 0,
        'evolve_bridges':     False, 'num_bridges':      0, 'bridge_width':  0.0,
        'evolve_barriers':    False, 'num_barriers':     0, 'barrier_width': 0.0,
    }

    for part in spec_str.split(';'):
        part = part.strip()
        m = re.match(r'([EHBX]):(.+)', part)
        if not m:
            continue
        letter, rest = m.group(1), m.group(2)

        if letter == 'E':
            m2 = re.match(r'(\d+),(\d+(?:\.\d+)?)', rest)
            if m2:
                result['evolve_entrances'] = True
                result['num_entrances']    = int(m2.group(1))
                result['entrance_width']   = float(m2.group(2))

        elif letter == 'H':
            m2 = re.match(r'(\d+),(\d+),(\d+)', rest)
            if m2:
                result['evolve_hives']        = True
                result['num_hives_inside']    = int(m2.group(1))
                result['num_hives_outside']   = int(m2.group(2))
                result['num_hives_free']      = int(m2.group(3))

        elif letter == 'B':
            m2 = re.match(r'(\d+),(\d+(?:\.\d+)?)', rest)
            if m2:
                result['evolve_bridges'] = True
                result['num_bridges']    = int(m2.group(1))
                result['bridge_width']   = float(m2.group(2))

        elif letter == 'X':
            m2 = re.match(r'(\d+),(\d+(?:\.\d+)?)', rest)
            if m2:
                result['evolve_barriers'] = True
                result['num_barriers']    = int(m2.group(1))
                result['barrier_width']   = float(m2.group(2))

    return result


# ---------------------------------------------------------------------------
# Header parsing
# ---------------------------------------------------------------------------

def _barrier_coords_to_cfg(s):
    """Convert 'x1,y1,x2,y2' (all commas) to cfg format 'x1,y1:x2,y2'."""
    parts = s.split(',')
    return f"{parts[0]},{parts[1]}:{parts[2]},{parts[3]}"


def _hive_log_to_cfg(val):
    """'(x,y):dir'  ->  'x,y:dir'"""
    m = re.match(r'\(([^)]+)\):(.*)', val)
    return f"{m.group(1)}:{m.group(2)}" if m else val


def _entrance_log_to_cfg(val):
    """'(e1,e2):side:netType'  ->  'e1,e2:side:netType'"""
    m = re.match(r'\(([^)]+)\):(.*)', val)
    return f"{m.group(1)}:{m.group(2)}" if m else val


def _barrier_log_to_cfg(val):
    """'(x1,y1,x2,y2):nrx,dx:nry,dy'  ->  'x1,y1:x2,y2:nrx,dx:nry,dy'"""
    m = re.match(r'\(([^)]+)\):(.*)', val)
    if m:
        return f"{_barrier_coords_to_cfg(m.group(1))}:{m.group(2)}"
    return val


def _patch_log_to_cfg(val):
    """
    '(x,y,w,h):sp:jit:species:nr:(dx,dy):ignore'
      -> 'x,y,w,h:sp:jit:species:nr:dx,dy:ignore'
    """
    m = re.match(r'\(([^)]+)\):([^:]+):([^:]+):([^:]+):([^:]+):\(([^)]+)\):(.*)', val)
    if m:
        return ':'.join([m.group(1), m.group(2), m.group(3),
                         m.group(4), m.group(5), m.group(6), m.group(7)])
    return val


def parse_header(lines):
    """
    Parse the '~~~~~~~~~~ FINAL PARAM VALUES ~~~~~~~~~~' block.

    Returns
    -------
    params     : OrderedDict-like list of (key, value) pairs (preserves order,
                 last value wins for duplicates such as evolve-spec)
    hives      : list of cfg-format hive strings  ('x,y:dir')
    entrances  : list of cfg-format entrance strings ('e1,e2:side:netType')
    barriers   : list of cfg-format barrier strings  ('x1,y1,x2,y2:...')
    patches    : list of cfg-format patch strings    ('x,y,w,h:...')
    """
    params    = {}          # key -> value (plain scalars)
    param_order = []        # to preserve output order
    hives     = []
    entrances = []
    barriers  = []
    patches   = []

    in_header = False

    for raw in lines:
        line = raw.rstrip()

        if line == '~~~~~~~~~~ FINAL PARAM VALUES ~~~~~~~~~~':
            in_header = True
            continue

        if in_header and line == '~~~~~~~~~~':
            break   # end of header block

        if not in_header:
            continue

        # Skip section dividers and empty-marker lines
        if line.startswith('~~~') or line == '(none)':
            continue

        # Match 'keyNAME: value' where NAME is an optional integer suffix
        m = re.match(r'^([\w-]+?)(\d*): (.*)', line)
        if not m:
            continue

        key = m.group(1)
        val = m.group(3)

        if key == 'config-filename':
            continue    # metadata, not a settable param

        if key == 'hive':
            hives.append(_hive_log_to_cfg(val))
        elif key == 'tunnel-entrance':
            entrances.append(_entrance_log_to_cfg(val))
        elif key == 'barrier':
            barriers.append(_barrier_log_to_cfg(val))
        elif key == 'patch':
            patches.append(_patch_log_to_cfg(val))
        else:
            if key not in params:
                param_order.append(key)
            params[key] = val   # last write wins (handles duplicate evolve-spec lines)

    return params, param_order, hives, entrances, barriers, patches


# ---------------------------------------------------------------------------
# Best-individual line parsing
# ---------------------------------------------------------------------------

def parse_best_individual_line(line, evolve_spec):
    """
    Extract evolved entrances, hives, bridge positions, and barrier endpoints
    from a fitness log line such as:

        INFO: isl 0 gen 5 evl 900 cnf 5 mdF -0.39400 /b/ b0 209.3,460.2 /x/ r0 100,200,150,250

    Returns a dict with keys 'entrances', 'hives', 'bridges', 'barriers',
    each containing a list of cfg-ready value strings.
    """
    result = {'entrances': [], 'hives': [], 'bridges': [], 'barriers': []}

    def extract_section(marker, item_re):
        """Return list of captured groups from item_re within the /marker/ section."""
        # Match everything between /marker/ and the next /x/ marker or end-of-string
        sec = re.search(r'/' + marker + r'/ (.*?)(?= /[a-z]/ |$)', line)
        if not sec:
            return []
        return re.findall(item_re, sec.group(1))

    if evolve_spec['evolve_entrances']:
        # Format: e0 e1,e2:side  (netType=0 assumed per spec)
        vals = extract_section('e', r'e\d+ (\S+)')
        result['entrances'] = [v + ':0' for v in vals]

    if evolve_spec['evolve_hives']:
        # Format: h0 x,y:dir
        result['hives'] = extract_section('h', r'h\d+ (\S+)')

    if evolve_spec['evolve_bridges']:
        # Format: b0 x,y   (top-left corner of the bridge patch)
        result['bridges'] = extract_section('b', r'b\d+ (\S+)')

    if evolve_spec['evolve_barriers']:
        # Format: r0 x1,y1,x2,y2
        result['barriers'] = extract_section('x', r'r\d+ (\S+)')

    return result


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main():
    args = parse_args()

    logfile = Path(args.logfile)
    if not logfile.exists():
        print(f"Error: log file not found: {logfile}", file=sys.stderr)
        sys.exit(1)

    with open(logfile, 'r') as f:
        lines = f.readlines()

    # --- Parse header ---
    params, param_order, base_hives, base_entrances, base_barriers, base_patches = \
        parse_header(lines)

    evolve_spec_str = params.get('evolve-spec', '')
    evolve_spec = parse_evolve_spec(evolve_spec_str)

    plant_spacing = params.get('plant-default-spacing', '10')
    plant_jitter  = params.get('plant-default-jitter', '0.1')

    # --- Find overall best fitness from second-to-last non-empty line ---
    nonempty = [l.rstrip() for l in lines if l.strip()]
    if len(nonempty) < 2:
        print("Error: log file too short to contain 'Overall best fitness'", file=sys.stderr)
        sys.exit(1)

    second_last = nonempty[-2]
    m = re.search(r'Overall best fitness: ([+-]?\d+\.\d+)', second_last)
    if not m:
        print(f"Error: could not parse 'Overall best fitness' from:\n  {second_last}",
              file=sys.stderr)
        sys.exit(1)

    best_fitness = m.group(1)
    print(f"Overall best fitness: {best_fitness}")

    # --- Find first log line that matches this fitness value ---
    fitness_pat = re.compile(
        r'INFO: isl \d+ gen \d+ evl \d+ cnf \d+ mdF ' + re.escape(best_fitness) + r' ')
    best_line = None
    for raw in lines:
        if fitness_pat.search(raw):
            best_line = raw.rstrip()
            break

    if best_line is None:
        print(f"Error: no log line found with mdF {best_fitness}", file=sys.stderr)
        sys.exit(1)

    print(f"Best individual line:\n  {best_line}")

    best = parse_best_individual_line(best_line, evolve_spec)

    # --- Apply mandatory overrides ---
    params['evolve']             = 'false'
    params['visualise']          = 'true'
    params['bee-path-record-len'] = '100'

    # --- Determine output path ---
    if args.output:
        outfile = Path(args.output)
    else:
        stem = logfile.stem
        if stem.startswith('out-'):
            stem = 'best-' + stem[4:]
        outfile = logfile.parent / (stem + '.cfg')

    # --- Write config file ---
    with open(outfile, 'w') as f:

        # Scalar parameters
        for key in param_order:
            f.write(f"{key}={params[key]}\n")

        f.write('\n')

        # Hives
        hives_out = best['hives'] if evolve_spec['evolve_hives'] else base_hives
        for h in hives_out:
            f.write(f"hive={h}\n")

        # Tunnel entrances
        entrances_out = best['entrances'] if evolve_spec['evolve_entrances'] else base_entrances
        for e in entrances_out:
            f.write(f"tunnel-entrance={e}\n")

        # Barriers: evolved values replace header values entirely
        if evolve_spec['evolve_barriers']:
            for b in best['barriers']:
                # b is 'x1,y1,x2,y2' from the log; cfg format requires 'x1,y1:x2,y2'
                f.write(f"barrier={_barrier_coords_to_cfg(b)}\n")
        else:
            for b in base_barriers:
                f.write(f"barrier={b}\n")

        # Patches: base patches always kept; evolved bridges appended
        for p in base_patches:
            f.write(f"patch={p}\n")

        if evolve_spec['evolve_bridges']:
            bw = evolve_spec['bridge_width']
            for pos in best['bridges']:
                # pos is 'x,y' (top-left corner of the bridge patch)
                f.write(f"patch={pos},{bw},{bw}:{plant_spacing}:{plant_jitter}:0:1:0,0:1\n")

    print(f"Config written to: {outfile}")


if __name__ == '__main__':
    main()
