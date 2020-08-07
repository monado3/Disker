import argparse
from subprocess import run
from pathlib import Path

p_proj = Path(__file__).parent.resolve()
p_exp = p_proj / 'exp'
p_bin = p_proj / 'bin'


def init_argparser():
    parser = argparse.ArgumentParser(
        prog='Disker',
        usage='pdfjs [-a <address>] [-b <block>] [-r <random>]',
        description='high functional Microbenchmark tool for disks',
        add_help=True,
    )
    parser.add_argument(
        '-a', '--address',
        help='measure throughput by adress',
        action='store_true',
        dest='address',
    )
    parser.add_argument(
        '-b', '--block',
        help='measure disk access by block size',
        action='store_true',
        dest='block',
    )
    parser.add_argument(
        '-r', '--random',
        help='measure random disk access by multiple conditions',
        action='store_true',
        dest='random',
    )
    return parser


def get_exp_idx() -> int:
    return len(list((p_exp.glob('./*')))) - 1


def get_exp_paths(kind):
    expidx = get_exp_idx()
    return p_exp / f'{expidx:02}{kind}', p_exp / f'{expidx:02}{kind}' / 'fig'


def measure_by_bsize():
    p_log, p_fig = get_exp_paths('bsize')
    p_log.mkdir(parents=True, exist_ok=False)
    run(f"{p_bin/ 'bsizer.out'} {p_log / 'log.csv'}", shell=True, check=True)


def measure_by_address():
    p_log, p_fig = get_exp_paths('address')
    p_log.mkdir(parents=True, exist_ok=False)
    run(f"{p_bin/ 'bsizer.out'} {p_log / 'log.csv'}", shell=True, check=True)


def measure_random():
    pass


def main():
    parser = init_argparser()
    args = parser.parse_args()
    if args.address:
        measure_by_address()
    if args.block:
        measure_by_bsize()
    if args.random:
        measure_random()


if __name__ == '__main__':
    main()
