Name:           slurm-jobmaker
Version:        1.0.0
Release:        1%{?dist}
Summary:        Graphical tool for generating Slurm job submission scripts

License:        MIT
URL:            https://github.com/vbrachetta/slurm-jobmaker
Source0:        %{name}-%{version}.tar.gz

BuildRequires:  cmake
BuildRequires:  gcc-c++
BuildRequires:  qt6-qtbase-devel
BuildRequires:  qt6-qtbase-gui

Requires:       qt6-qtbase
Requires:       qt6-qtbase-gui

%description
Slurm JobMaker is an open-source, portable graphical utility for the
interactive generation of Slurm job submission scripts. Designed to
support early-stage researchers and occasional HPC users, it guides
users through the configuration of key job parameters and displays a
live preview of the resulting script. Cluster-specific settings are
loaded from plain-text profile files, making the tool adaptable to
any Slurm-based HPC system without recompilation.

%prep
%autosetup

%build
rm -rf build
cmake -B build \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_INSTALL_PREFIX=/usr
cmake --build build --parallel $(nproc)

%install
install -Dm755 build/SlurmJobMaker \
        %{buildroot}/usr/bin/SlurmJobMaker
install -Dm644 profiles/bluebear.conf \
        %{buildroot}/usr/share/%{name}/profiles/bluebear.conf
install -Dm644 profiles/generic.conf \
        %{buildroot}/usr/share/%{name}/profiles/generic.conf
install -Dm644 assets/icons/16x16/slurm-jobmaker.png \
        %{buildroot}/usr/share/icons/hicolor/16x16/apps/slurm-jobmaker.png
install -Dm644 assets/icons/32x32/slurm-jobmaker.png \
        %{buildroot}/usr/share/icons/hicolor/32x32/apps/slurm-jobmaker.png
install -Dm644 assets/icons/48x48/slurm-jobmaker.png \
        %{buildroot}/usr/share/icons/hicolor/48x48/apps/slurm-jobmaker.png
install -Dm644 assets/icons/128x128/slurm-jobmaker.png \
        %{buildroot}/usr/share/icons/hicolor/128x128/apps/slurm-jobmaker.png
install -Dm644 assets/icons/256x256/slurm-jobmaker.png \
        %{buildroot}/usr/share/icons/hicolor/256x256/apps/slurm-jobmaker.png

%files
/usr/bin/SlurmJobMaker
/usr/share/%{name}/profiles/bluebear.conf
/usr/share/%{name}/profiles/generic.conf
/usr/share/icons/hicolor/16x16/apps/slurm-jobmaker.png
/usr/share/icons/hicolor/32x32/apps/slurm-jobmaker.png
/usr/share/icons/hicolor/48x48/apps/slurm-jobmaker.png
/usr/share/icons/hicolor/128x128/apps/slurm-jobmaker.png
/usr/share/icons/hicolor/256x256/apps/slurm-jobmaker.png

%changelog
* Fri Apr 17 2026 Vincenzo Brachetta <v.brachetta@bham.ac.uk> - 1.0.0-1
- Initial release
