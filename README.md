# systemd for ubuntu with dbus enabled

This repo is intented to provide a unstable but compilable source for [miraclecast](https://github.com/albfan/miraclecast) systemd dependency on ubuntu

See this repo rebase its patches from original upstream (as upstream changes frequently and base for patches get obsolete)

# Background

This is a clone patched from systemd patched code for ubuntu created from systemd patched code for debian created from freedesktop systemd original source

# usage

To apply this patches and generate .deb files you need to:

 - get ubuntu systemd sources:

        $ mkdir systemd
        $ cd systemd
        $ apt-get source systemd

 - replace extracted sources with this repo sources:

        $ rm -rf systemd-219
        $ mkdir systemd-219
        $ cd systemd-219
        $ git clone https://github.com/albfan/systemd-ubuntu-with-dbus

 - generate a patch in debian/patches
        $ dpkg-source --commit
          # choose name for patch

 - continue with compilation and packaging

        $ dpkg-buildpackage -uc -us -D -rfakeroot

 - install all .deb generated

    this process is really manual, so much that you must do this to install correctly

        $ rm /usr/share/doc/libsystemd0/changelog.Debian.gz 
        $ rm /usr/share/doc/libudev1/changelog.Debian.gz 

        $ sudo dpkg -i *.deb
    
# Original repos

- freedesktop: 

  - web: http://www.freedesktop.org/software/systemd/
  - vcs: git://anongit.freedesktop.org/systemd/systemd

- debian

  - web: https://anonscm.debian.org/cgit/pkg-systemd/systemd.git
  - vcs: git://anonscm.debian.org/pkg-systemd/systemd.git

- ubuntu
  - web: https://launchpad.net/ubuntu/+source/systemd/219-4ubuntu9
  - vcs: lp:ubuntu/systemd
