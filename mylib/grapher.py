import matplotlib.pyplot as plt
import pandas as pd
from humanize import naturalsize


def natsize(value, **kwargs):
    return naturalsize(value, binary=True, **kwargs)


def vec_natsize(lis):
    return [natsize(x, format='%.0f') for x in lis]


def const_accesser(df, lis):
    return [df.loc[0, x] for x in lis]


def zeropointfy(ax):
    bt, top = ax.get_ylim()
    if bt > 0:
        ax.set_ylim(0)


def plt_settings():
    plt.rcParams['font.size'] = 8

    plt.rcParams['xtick.direction'] = 'in'
    plt.rcParams['ytick.direction'] = 'in'
    plt.rcParams['xtick.major.width'] = 1.2
    plt.rcParams['ytick.major.width'] = 1.2
    plt.rcParams['axes.linewidth'] = 1.2
    plt.rcParams['axes.grid'] = True
    plt.rcParams['grid.linestyle'] = '--'
    plt.rcParams['grid.linewidth'] = 0.5

    plt.rcParams["legend.markerscale"] = 2
    plt.rcParams["legend.fancybox"] = False
    plt.rcParams["legend.framealpha"] = 1
    plt.rcParams["legend.edgecolor"] = 'black'


def _base_tp_iops_fig(df, x):
    fig, (axtp, axiops) = plt.subplots(
        2, 1, sharex=True, figsize=(10, 10), dpi=250)

    axtp.plot(df[x], df['direct_tp(MB/sec)'], label='w/ O_DIRECT', marker='+')
    axtp.plot(df[x], df['indirect_tp(MB/sec)'],
              label='w/o O_DIRECT', marker='x')
    axiops.plot(df[x], df['direct_iops'], label='w/ O_DIRECT', marker='+')
    axiops.plot(df[x], df['indirect_iops'], label='w/o O_DIRECT', marker='x')

    zeropointfy(axtp)
    zeropointfy(axiops)
    axtp.legend()
    axiops.legend()
    axtp.set_ylabel('throughput (MB/sec)')
    axiops.set_ylabel('IOPS')
    return fig, (axtp, axiops)


def _set_bsize_xtick(axiops, df):
    axiops.set_xscale('log')
    axiops.set_xticks(df['bsize'])
    axiops.set_xticklabels(vec_natsize(
        df['bsize']), fontsize='small', rotation=45)
    axiops.set_xlabel('block size (Bytes)')
    return axiops


def read_s_bsizes(p):
    return pd.read_csv(p / 'log.csv', skiprows=3)


def plot_s_bsizes(df):
    nthreads, readarea = const_accesser(df, ['nthreads', 'readbytes'])

    fig, (axtp, axiops) = _base_tp_iops_fig(df, 'bsize')
    axiops = _set_bsize_xtick(axiops, df)

    axtp.set_title(
        f'Seq. Access Microbenchmark by block size\n({nthreads=}, readarea={natsize(readarea)})')
    return fig


def read_s_addresses(p):
    return pd.read_csv(p / 'log.csv', skiprows=3)


def plot_s_addresses(df):
    bsize, nthreads, readarea = const_accesser(
        df, ['bsize', 'nthreads', 'readbytes'])

    fig, ax = plt.subplots(figsize=(10, 5), dpi=250)

    ax.plot(df['address'], df['direct_tp(MB/sec)'],
            label='w/ O_DIRECT', linewidth=0.5)

    zeropointfy(ax)
    ax.legend()
    ax.set_xlabel('address')
    ax.set_ylabel('throughput (MB/sec)')
    ax.set_title(
        f'Seq. Access Microbenchmark by address\n(bsize={natsize(bsize)}, {nthreads=}, readarea={natsize(readarea)})')
    return fig


def read_r_bsizes(p):
    return pd.read_csv(p / '01bsize.csv', skiprows=3)


def plot_r_bsizes(df):
    region, nthreads, nreads = const_accesser(
        df, ['region', 'nthreads', 'nreads'])

    fig, (axtp, axiops) = _base_tp_iops_fig(df, 'bsize')
    axiops = _set_bsize_xtick(axiops, df)

    axtp.set_title(
        f'Rnd. Access Microbenchmark by block size\n({region=:.1f}, {nthreads=} {nreads=})')
    return fig


def read_r_regions(p):
    return pd.read_csv(p / '02region.csv', skiprows=3)


def plot_r_regions(df):
    bsize, nthreads, nreads = const_accesser(
        df,  ['bsize', 'nthreads', 'nreads'])

    fig, (axtp, axiops) = _base_tp_iops_fig(df, 'region')

    axtp.set_xscale('log')
    axiops.set_xlabel('region (ratio)')
    axtp.set_title(
        f'Rnd. Access Microbenchmark by region ratio\n(bsize={natsize(bsize)}, {nthreads=} {nreads=})')
    return fig


def read_r_threads(p):
    return pd.read_csv(p / '03threads.csv', skiprows=3)


def plot_r_threads(df):
    bsize, region, nreads = const_accesser(df, ['bsize', 'region', 'nreads'])

    fig, (axtp, axiops) = _base_tp_iops_fig(df, 'nthreads')

    axiops.set_xlabel('No. of threads')
    axtp.set_title(
        f'Rnd. Access Microbenchmark by No. of threads\n(bsize={natsize(bsize)}, {region=:.1f}, {nreads=})')
    return fig


def read_r_regions_mthreads(p):
    return pd.read_csv(p / '04regionmulti.csv', skiprows=3)


def plot_r_regions_mthreads(df):
    bsize, nthreads, nreads = const_accesser(
        df, ['bsize', 'nthreads', 'nreads'])

    fig, (axtp, axiops) = _base_tp_iops_fig(df, 'region')

    axtp.set_xscale('log')
    axiops.set_xlabel('region (ratio)')
    axtp.set_title(
        f'Rnd. Access Microbenchmark by region ratio\n(bsize={natsize(bsize)}, {nthreads=} {nreads=})')
    return fig
