PACKAGE=qt
$(package)_version=5.7.1
$(package)_download_path=http://download.qt.io/official_releases/qt/5.7/$($(package)_version)/submodules
$(package)_suffix=opensource-src-$($(package)_version).tar.gz
$(package)_file_name=qtbase-$($(package)_suffix)
$(package)_sha256_hash=95f83e532d23b3ddbde7973f380ecae1bac13230340557276f75f2e37984e410
$(package)_dependencies=openssl zlib
$(package)_linux_dependencies=freetype fontconfig libxcb libX11 xproto libXext
$(package)_build_subdir=qtbase
$(package)_qt_libs=corelib network widgets gui plugins testlib
$(package)_patches=mac-qmake.conf aarch32-qmake.conf aarch64-qmake.conf mingw-uuidof.patch pidlist_absolute.patch fix-xcb-include-order.patch fix_qt_pkgconfig.patch

$(package)_qttranslations_file_name=qttranslations-$($(package)_suffix)
$(package)_qttranslations_sha256_hash=3a15aebd523c6d89fb97b2d3df866c94149653a26d27a00aac9b6d3020bc5a1d


$(package)_qttools_file_name=qttools-$($(package)_suffix)
$(package)_qttools_sha256_hash=22d67de915cb8cd93e16fdd38fa006224ad9170bd217c2be1e53045a8dd02f0f

$(package)_extra_sources  = $($(package)_qttranslations_file_name)
$(package)_extra_sources += $($(package)_qttools_file_name)

define $(package)_set_vars
$(package)_config_opts_release = -release
$(package)_config_opts_debug = -debug
$(package)_config_opts += -bindir $(build_prefix)/bin
$(package)_config_opts += -c++std c++11
$(package)_config_opts += -confirm-license
$(package)_config_opts += -dbus-runtime
$(package)_config_opts += -hostprefix $(build_prefix)
$(package)_config_opts += -no-alsa
$(package)_config_opts += -no-audio-backend
$(package)_config_opts += -no-cups
$(package)_config_opts += -no-egl
$(package)_config_opts += -no-eglfs
$(package)_config_opts += -no-feature-style-windowsmobile
$(package)_config_opts += -no-feature-style-windowsce
$(package)_config_opts += -no-freetype
$(package)_config_opts += -no-gif
$(package)_config_opts += -no-glib
$(package)_config_opts += -no-gstreamer
$(package)_config_opts += -no-icu
$(package)_config_o