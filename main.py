import argparse
from enum import Enum
from subprocess import run
from pathlib import Path

from mylib.grapher import *

p_proj = Path(__file__).parent.resolve()
p_exps = p_proj / 'exp'
p_bin = p_proj / 'bin'


class RW(Enum):
    READ = 'read'
    WRITE = 'write'


def init_argparser():
    parser = argparse.ArgumentParser(
        prog='Disker',
        usage='python main.py [--read] [--write] [-a <address>] [-b <block>] [-r <random>]',
        description='high functional Microbenchmark tool for disks',
        add_help=True,
    )
    group = parser.add_argument_group('Set Measure Mode')
    group.add_argument('--read',
                       help='do read test',
                       action='store_true',
                       dest='read',
                       )
    group.add_argument('--write',
                       help='do write test',
                       action='store_true',
                       dest='write',
                       )

    parser.add_argument('-a', '--address',
                        help='measure throughput by adress',
                        action='store_true',
                        dest='address',
                        )
    parser.add_argument('-b', '--block',
                        help='measure disk access by block size',
                        action='store_true',
                        dest='block',
                        )
    parser.add_argument('-r', '--random',
                        help='measure random disk access by multiple conditions',
                        action='store_true',
                        dest='random',
                        )
    return parser


def check_args(args):
    if not (args.read or args.write):
        print('You need to do at least one mode (--read or --write)')
    if not (args.address or args.block or args.random):
        print('You need to specify at least one measuremnt item (-a, -b, -r)')


def get_exp_idx() -> int:
    return len(list((p_exps.glob('./*')))) - 1


def get_exp_paths(kind):
    expidx = get_exp_idx()
    return p_exps / f'{expidx:02}{kind}', p_exps / f'{expidx:02}{kind}' / 'fig'


def measure_by_bsize(rw):
    p_exp, p_fig = get_exp_paths(f'{rw.value}_bsize')
    p_exp.mkdir(parents=True, exist_ok=False)
    run(f"{p_bin/ 'bsizer.out'} {rw.value} {p_exp / 'log.csv'}",
        shell=True, check=True)

    p_fig.mkdir(parents=True, exist_ok=False)
    df = read_s_bsizes(p_exp)
    fig = plot_s_bsizes(df)
    fig.savefig(p_fig / f'{rw.value}_s_bsize.png')


def measure_by_address(rw):
    p_exp, p_fig = get_exp_paths(f'{rw.value}_address')
    p_exp.mkdir(parents=True, exist_ok=False)
    run(f"{p_bin/ 'addresser.out'} {rw.value} {p_exp / 'log.csv'}",
        shell=True, check=True)

    p_fig.mkdir(parents=True, exist_ok=False)
    df = read_s_addresses(p_exp)
    fig = plot_s_addresses(df)
    fig.savefig(p_fig / f'{rw.value}_s_address.png')


def measure_random(rw):
    p_exp, p_fig = get_exp_paths(f'{rw.value}_random')
    p_log = p_exp / 'log'
    p_log.mkdir(parents=True, exist_ok=False)
    run(f"{p_bin/ 'randomer.out'} {rw.value} {p_log}", shell=True, check=True)
    p_fig.mkdir(parents=True, exist_ok=False)

    df = read_r_bsizes(p_log)
    fig = plot_r_bsizes(df)
    fig.savefig(p_fig / f'{rw.value}_r_01bsize.png')

    df = read_r_regions(p_log)
    fig = plot_r_regions(df)
    fig.savefig(p_fig / f'{rw.value}_r_02region.png')

    df = read_r_threads(p_log)
    fig = plot_r_threads(df)
    fig.savefig(p_fig / f'{rw.value}_r_03threads.png')

    df = read_r_regions_mthreads(p_log)
    fig = plot_r_regions_mthreads(df)
    fig.savefig(p_fig / f'{rw.value}_r_04regions_mthreads.png')


def main():
    parser = init_argparser()
    args = parser.parse_args()
    check_args(args)

    plt_settings()

    if args.read:
        if args.block:
            measure_by_bsize(RW.READ)
        if args.random:
            measure_random(RW.READ)
        if args.address:
            measure_by_address(RW.READ)
    if args.write:
        if args.block:
            measure_by_bsize(RW.WRITE)
        if args.random:
            measure_random(RW.WRITE)
        if args.address:
            measure_by_address(RW.WRITE)


if __name__ == '__main__':
    main()
