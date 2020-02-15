package=native_cctools
$(package)_version=807d6fd1be5d2224872e381870c0a75387fe05e6
$(package)_download_path=https://github.com/theuni/cctools-port/archive
$(package)_file_name=$($(package)_version).tar.gz
$(package)_sha256_hash=a09c9ba4684670a0375e42d9d67e7f12c1f62581a27f28f7c825d6d7032ccc6a
$(package)_build_subdir=cctools
$(package)_clang_version=3.7.1
$(package)_clang_download_path=http://llvm.org/releases/$($(package)_clang_version)
$(package)_clang_download_file=clang+llvm-$($(package)_clang_version)-x86_64-linux-gnu-ubuntu-14.04.tar.xz
$(package)_clang_file_name=clang-llvm-$($(package)_clang_version)-x86_64-linux-gnu-ubuntu-14.04.tar.xz
$(package)_clang_sha256_hash=99b28a6b48e793705228a390471991386daa33a9717cd9ca007fcdde69608fd9
$(package)_extra_sources=$($(package)_clang_file_name)

define $(package)_fetch_cmds
