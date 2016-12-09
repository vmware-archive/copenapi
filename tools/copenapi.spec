Name:    copenapi
Summary: c open api spec parser
Version: 0.1
Release: 1
Group:   Development/Libraries
Vendor:  VMware, Inc.
License: VMware
URL:     http://www.vmware.com
BuildArch: x86_64
Requires: jansson
BuildRequires: make
Source0: %{name}-%{version}.tar.gz

%description
copenapi is an openapi parser written in c

%prep
%setup -q

%build
autoreconf -mif
./configure \
    --prefix=%{_prefix}

make

%install

make install DESTDIR=$RPM_BUILD_ROOT

%post
    /sbin/ldconfig

%files
%defattr(-,root,root)
%{_bindir}/*
%{_libdir}/*
%{_libdir}/pkgconfig/copenapi.pc
%{_includedir}/*

%changelog
*  Wed Oct 05 2016 Priyesh Padmavilasom<ppadmavilasom@vmware.com> 0.1-1
-  Initial
