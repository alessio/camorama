# gcc compatibility for redhat 7.x if using RawHide or 8.x pre
%define old_gcc296 0

%define glib2_version 2.0.0
%define pango_version 1.0.3
%define gtk2_version 2.0.3
%define libgnome_version 2.0.0
%define libgnomeui_version 2.0.1
%define libbonobo_version 2.0.0
%define libbonoboui_version 2.0.0
%define gnome_vfs2_version 2.0.0
%define bonobo_activation_version 1.0.0
%define libpng_version 1.2.2

%define po_package gnome-session-2.0

Summary: GNOME webcam program
Name: camorama
Version: 0.17
Release: 1
URL: http://camorama.fixedgear.org
Source0: %{name}-%{version}.tar.bz2
License: GPL 
Group: User Interface/Desktops
BuildRoot: %{_tmppath}/%{name}-%{version}-root

BuildRequires: glib2-devel >= %{glib2_version}
BuildRequires: pango-devel >= %{pango_version}
BuildRequires: gtk2-devel >= %{gtk2_version}
BuildRequires: libgnome-devel >= %{libgnome_version}
BuildRequires: libgnomeui-devel >= %{libgnomeui_version}
BuildRequires: libbonobo-devel >= %{libbonobo_version}
BuildRequires: libbonoboui-devel >= %{libbonoboui_version}
BuildRequires: gnome-vfs2-devel >= %{gnome_vfs2_version}
BuildRequires: bonobo-activation-devel >= %{bonobo_activation_version}
BuildRequires: libpng-devel >= %{libpng_version}

%description

Camorama is a program for controlling webcams.  It is pretty simple at the moment, and I hope to make it much more complete.  I also plan to make it more generic, as I initially wrote it with only my own Creative Webcam 3 in mind.  Hopefully it will work with other cameras.


%prep
%setup -q

%build

aclocal
autoconf
automake --add-missing --force

%configure
# Compiler Munge
%if %old_gcc296
make CC=gcc296 CXX=g++296
%else
make %_smp_mflags
%endif

%install
rm -rf $RPM_BUILD_ROOT

make DESTDIR=$RPM_BUILD_ROOT install

%clean
rm -rf $RPM_BUILD_ROOT

%post

%files 
%defattr(-,root,root)

%doc AUTHORS ChangeLog COPYING MAINTAINERS NEWS README THANKS TODO

%{_datadir}/applications/camorama.desktop
%{_datadir}/pixmaps/camorama.png
%{_datadir}/camorama/*
%{_datadir}/locale/am/LC_MESSAGES/camorama.mo
%{_datadir}/locale/be/LC_MESSAGES/camorama.mo
%{_datadir}/locale/cs/LC_MESSAGES/camorama.mo
%{_datadir}/locale/da/LC_MESSAGES/camorama.mo
%{_datadir}/locale/de/LC_MESSAGES/camorama.mo
%{_datadir}/locale/es/LC_MESSAGES/camorama.mo
%{_datadir}/locale/fa/LC_MESSAGES/camorama.mo
%{_datadir}/locale/fr/LC_MESSAGES/camorama.mo
%{_datadir}/locale/ja/LC_MESSAGES/camorama.mo
%{_datadir}/locale/ml/LC_MESSAGES/camorama.mo
%{_datadir}/locale/nl/LC_MESSAGES/camorama.mo
%{_datadir}/locale/no/LC_MESSAGES/camorama.mo
%{_datadir}/locale/pl/LC_MESSAGES/camorama.mo
%{_datadir}/locale/pt/LC_MESSAGES/camorama.mo
%{_datadir}/locale/pt_BR/LC_MESSAGES/camorama.mo
%{_datadir}/locale/sr/LC_MESSAGES/camorama.mo
%{_datadir}/locale/sr@Latn/LC_MESSAGES/camorama.mo
%{_datadir}/locale/sv/LC_MESSAGES/camorama.mo
%{_datadir}/locale/uk/LC_MESSAGES/camorama.mo
%{_datadir}/locale/vi/LC_MESSAGES/camorama.mo
%{_bindir}/camorama
%{_sysconfdir}/gconf/schemas/camorama.schemas

%changelog
* Wed Aug 20 2003 Kyle Gonzales <kgonzales@rev.net> 0.17-rh1
- Upped to version 0.17; see CHANGELOG for details
- Minor RPM changes to accomodate the new version

* Sat Jan 19 2003 Kyle Gonzales <kgonzales@rev.net> 0.16-rh1
- Upped to version 0.16; see CHANGELOG for details
- Minor RPM changes to accomodate the new version

* Tue Jan 14 2003 Kyle Gonzales <kgonzales@rev.net> 0.15a-rh1
- Changed to new version that will work on Red Hat 8.0
- Added libpng-devel as a build requirement

* Wed Sep 11 2002 Kirk Whiting <kirk@death-linux.cc> 0.14c-1
- Upped to new version

* Wed Aug 28 2002 Kirk Whiting <kirk@death-linux.cc> 0.14-2
- fixed pkgconfig search for imlibgdk instead of gdk_imlib

* Wed Aug 28 2002 Kirk Whiting <kirk@death-linux.cc> 0.14-1
- upped version to 0.14

* Mon Aug 5 2002 Kirk Whiting <kirk@death-linux.cc> 0.13-2
- specfile cleanups, gnome2 deps.

* Fri Aug 2 2002 Kirk Whiting <kirk@death-linux.cc> 0.13-1
- initial spec
