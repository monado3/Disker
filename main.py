import argparse
from subprocess import run
from pathlib import Path

from .mylib.grapher import *

p_proj = Path(__file__).parent.resolve()
p_exps = p_proj / 'exp'
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
    return len(list((p_exps.glob('./*')))) - 1


def get_exp_paths(kind):
    expidx = get_exp_idx()
    return p_exps / f'{expidx:02}{kind}', p_exps / f'{expidx:02}{kind}' / 'fig'


def measure_by_bsize():
    p_exp, p_fig = get_exp_paths('bsize')
    p_exp.mkdir(parents=True, exist_ok=False)
    run(f"{p_bin/ 'bsizer.out'} {p_exp / 'log.csv'}", shell=True, check=True)

    p_fig.mkdir(parents=True, exist_ok=False)
    df = read_s_bsizes(p_exp)
    fig = plot_s_bsizes(df)
    fig.savefig(p_fig / 's_bsize.png')


def measure_by_address():
    p_exp, p_fig = get_exp_paths('address')
    p_exp.mkdir(parents=True, exist_ok=False)
    run(f"{p_bin/ 'addresser.out'} {p_exp / 'log.csv'}", shell=True, check=True)

    p_fig.mkdir(parents=True, exist_ok=False)
    df = read_s_addresses(p_exp)
    fig = plot_s_addresses(df)
    fig.savefig(p_fig / 's_address.png')


def measure_random():
    p_exp, p_fig = get_exp_paths('random')
    p_log = p_exp / 'log'
    p_log.mkdir(parents=True, exist_ok=False)
    run(f"{p_bin/ 'randomer.out'} {p_log}", shell=True, check=True)
    p_fig.mkdir(parents=True, exist_ok=False)

    df = read_r_bsizes(p_log)
    fig = plot_r_bsizes(df)
    fig.savefig(p_fig / 'r_01bsize.png')

    df = read_r_regions(p_log)
    fig = plot_r_regions(df)
    fig.savefig(p_fig / 'r_02region.png')

    df = read_r_threads(p_log)
    fig = plot_r_threads(df)
    fig.savefig(p_fig / 'r_03threads.png')

    df = read_r_regions_mthreads(p_log)
    fig = plot_r_regions_mthreads(df)
    fig.savefig(p_fig / 'r_04regions_mthreads.png')


def main():
    parser = init_argparser()
    args = parser.parse_args()

    plt_settings()

    if args.block:
        measure_by_bsize()
    if args.random:
        measure_random()
    if args.address:
        measure_by_address()


if __name__ == '__main__':
    main()
