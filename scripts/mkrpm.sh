#! /bin/sh
#
# $1 = temporary directory for RPM
# $2 = kernel version number to build for
# $3 = output RPM name

if [ $# -ne 2 ]; then
	echo "Syntax:  $0 wish_ver kernel_ver"
	exit 1
fi

WISHVER="$1"
KERNEL="$2"

BUILDDIR="/tmp/$KERNEL-buildroot"
rm -rf $BUILDDIR
mkdir -p $BUILDDIR
tar cf - . | (cd $BUILDDIR; tar xf -)

echo "...creating spec file"

cat << END_OF_SPEC > x10.spec
Summary:  Linux X10 device drivers for PowerLinc USB, PowerLinc Serial, CM11A, and CM17A
Name:  wish-linux-$KERNEL
Version: $WISHVER
Release: 0
Copyright:  GPL
Group:  System Environment/Kernel
BuildRoot: $BUILDDIR
Vendor:  Scott Hiles (wsh@sprintmail.com)

%description
Device drivers for PowerLinc USB, PowerLinc Serial, CM11A, and CM17A X10 transceivers

%prep

%build
cd \$RPM_BUILD_ROOT
make KERNELDIR=/usr/src/linux-$KERNEL

%install
cd \$RPM_BUILD_ROOT
mkdir -p \$RPM_BUILD_ROOT/usr/share/doc/wish-$WISHVER
cp html/* \$RPM_BUILD_ROOT/usr/share/doc/wish-$WISHVER
mkdir -p \$RPM_BUILD_ROOT/lib/modules/$KERNEL/kernel/drivers/char/x10
cp x10_plusb.o x10_pl.o x10_cm11a.o x10_cm17a.o \$RPM_BUILD_ROOT/lib/modules/$KERNEL/kernel/drivers/char/x10/
mkdir -p \$RPM_BUILD_ROOT/usr/local/etc/
cp example_scripts/x10.cm11a.sh \$RPM_BUILD_ROOT/usr/local/etc/x10_cm11a
cp example_scripts/x10.cm17a.sh \$RPM_BUILD_ROOT/usr/local/etc/x10_cm17a
cp example_scripts/x10.plusb.sh \$RPM_BUILD_ROOT/usr/local/etc/x10_plusb
cp example_scripts/x10.pl.sh \$RPM_BUILD_ROOT/usr/local/etc/x10_pl
mkdir -p \$RPM_BUILD_ROOT/tmp
cp scripts/makedev.sh \$RPM_BUILD_ROOT/tmp
mkdir -p \$RPM_BUILD_ROOT/usr/sbin
mkdir -p \$RPM_BUILD_ROOT/usr/bin
install -o root utils/x10attach \$RPM_BUILD_ROOT/usr/sbin/x10attach
install -o root utils/x10logd \$RPM_BUILD_ROOT/usr/sbin/x10logd
install -o root utils/nbread \$RPM_BUILD_ROOT/usr/bin/nbread
install -o root utils/x10watch \$RPM_BUILD_ROOT/usr/bin/x10watch
mkdir -p \$RPM_BUILD_ROOT/dev/x10
./scripts/makedev.sh 120 121 \$RPM_BUILD_ROOT/dev/x10

%post
/tmp/makedev.sh

%clean
cd \$RPM_BUILD_ROOT
make clean

%files
%defattr(-,root,root)
/usr/sbin/x10attach
/usr/sbin/x10logd
/usr/bin/nbread
/usr/bin/x10watch
/lib/modules/$KERNEL/kernel/drivers/char/x10/x10_plusb.o
/lib/modules/$KERNEL/kernel/drivers/char/x10/x10_pl.o
/lib/modules/$KERNEL/kernel/drivers/char/x10/x10_cm11a.o
/lib/modules/$KERNEL/kernel/drivers/char/x10/x10_cm17a.o
/usr/local/etc/x10_cm11a
/usr/local/etc/x10_cm17a
/usr/local/etc/x10_plusb
/usr/local/etc/x10_pl
/usr/share/doc/wish-$WISHVER/*
/tmp/makedev.sh
END_OF_SPEC

echo "... building RPM package"
rpm -bb x10.spec
