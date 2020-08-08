import matplotlib.pyplot as plt
import pandas as pd


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

    axtp.plot(df[x], df['direct_tp(MB/sec)'], label='w/ O_DIRECT')
    axtp.plot(df[x], df['indirect_tp(MB/sec)'], label='w/o O_DIRECT')
    axiops.plot(df[x], df['direct_iops'], label='w/ O_DIRECT')
    axiops.plot(df[x], df['indirect_iops'], label='w/o O_DIRECT')

    axtp.legend()
    axiops.legend()
    axtp.set_ylabel('throughput (MB/sec)')
    axiops.set_ylabel('IOPS')
    return fig, (axtp, axiops)


def read_s_bsizes(p):
    return pd.read_csv(p / 'log.csv', skiprows=3)


def plot_s_bsizes(df):
    fig, (axtp, axiops) = _base_tp_iops_fig(df, 'bsize')

    axtp.set_xscale('log')
    axiops.set_xlabel('block size (Bytes)')
    axtp.set_title('Seq. Access Microbenchmark (by block size)')
    return fig


def read_s_addresses(p):
    return pd.read_csv(p / 'log.csv', skiprows=3)


def plot_s_addresses(df):
    fig, ax = plt.subplots(figsize=(10, 5), dpi=250)

    ax.plot(df['address'], df['direct_tp(MB/sec)'], label='w/ O_DIRECT')

    ax.legend()
    ax.set_xlabel('address')
    ax.set_ylabel('throughput (MB/sec)')
    ax.set_title('Seq. Access Microbenchmark (by address)')
    return fig


def read_r_bsizes(p):
    return pd.read_csv(p / '01bsize.csv', skiprows=3)


def plot_r_bsizes(df):
    fig, (axtp, axiops) = _base_tp_iops_fig(df, 'bsize')

    axtp.set_xscale('log')
    axiops.set_xlabel('block size (Bytes)')
    axtp.set_title('Rand. Access Microbenchmark (by block size)')
    return fig


def read_r_regions(p):
    return pd.read_csv(p / '02region.csv', skiprows=3)


def plot_r_regions(df):
    fig, (axtp, axiops) = _base_tp_iops_fig(df, 'region')

    axtp.set_xscale('log')
    axiops.set_xlabel('region (ratio)')
    axtp.set_title('Rand. Access Microbenchmark (by region ratio)')
    return fig


def read_r_threads(p):
    return pd.read_csv(p / '03threads.csv', skiprows=3)


def plot_r_threads(df):
    fig, (axtp, axiops) = _base_tp_iops_fig(df, 'nthreads')

    axiops.set_xlabel('No. of threads')
    axtp.set_title('Rand. Access Microbenchmark (by No. of threads)')
    return fig


def read_r_regions_mthreads(p):
    return pd.read_csv(p / '04regionmulti.csv', skiprows=3)


def plot_r_regions_mthreads(df):
    nthreads = df.loc[0, 'nthreads']

    fig, (axtp, axiops) = _base_tp_iops_fig(df, 'region')

    axtp.set_xscale('log')
    axiops.set_xlabel('region (ratio')
    axtp.set_title(
        f'Rand. Access Microbenchmark (by region ratio in {nthreads} threads)')
    return fig
