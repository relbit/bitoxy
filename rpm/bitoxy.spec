Name:		bitoxy
Version:	0.4.0
Release:	1%{?dist}
Summary:	FTP/FTPS proxy

Group:		Applications/Internet
License:	Apache2
URL:		http://relbit.com
Source0:	bitoxy-%{version}.tar.gz
BuildRoot:	%(mktemp -ud %{_tmppath}/%{name}-%{version}-%{release}-XXXXXX)

BuildRequires:	qt-devel
Requires:	qt qt-mysql

%description
FTP/FTPS proxy

%prep
%setup -q


%build
#cd bitoxy-%{version}
qmake-qt4 "CONFIG+=qt debug"
make %{?_smp_mflags}


%install
#rm -rf $RPM_BUILD_ROOT
#make install DESTDIR=$RPM_BUILD_ROOT
install -m 0755 -D bitoxy %{buildroot}/%{_bindir}/bitoxy
install -m 0600 -D bitoxy.conf %{buildroot}/%{_sysconfdir}/bitoxy.conf
install -m 0755 -D rpm/bitoxy.init %{buildroot}/%{_sysconfdir}/init.d/bitoxy
install -m 0644 -D rpm/bitoxy.sysconf %{buildroot}/%{_sysconfdir}/sysconfig/bitoxy

%clean
rm -rf $RPM_BUILD_ROOT


%files
%defattr(-,root,root,-)
%doc
%{_bindir}/bitoxy
%config(noreplace) %{_sysconfdir}/bitoxy.conf
%{_sysconfdir}/init.d/bitoxy
%{_sysconfdir}/sysconfig/bitoxy

%changelog
* Thu Mar 05 2014 - Jakub Skokan <aither@havefun.cz> - 0.4.0
- New bitoxy version - access logs, debug output

* Thu Feb 13 2014 - Jakub Skokan <aither@havefun.cz> - 0.3.3
- Add debug info

* Fri Jan 19 2013 - Jakub Skokan <aither@havefun.cz> - 0.3.3
- New version

* Fri Oct 26 2012 - Jakub Skokan <aither@havefun.cz> - 0.3.0
- Init script

* Fri Aug 24 2012 - Jakub Skokan <aither@havefun.cz> - 0.2.0
- New version

* Wed Aug 8 2012 - Jakub Skokan <aither@havefun.cz> - 0.1.0
- Initial version

