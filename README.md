# systemd for ubuntu with dbus enabled

This repo is intented to provide a compilable source for [miraclecast](https://github.com/albfan/miraclecast) systemd dependency on ubuntu

It is based on official git repo on debian

## Usage

- Create a sandbox
   
        $ mkdir systemd-sandbox
        $ cd systemd-sandbox

- Clone this repo

        $ git clone https://github.com/albfan/systemd-ubuntu-with-dbus

- Add upstream remotes

        $ cd systemd-ubuntu-with-dbus  
        $ git remote add freedesktop git://anongit.freedesktop.org/systemd/systemd
        $ git remote add debian git://anonscm.debian.org/pkg-systemd/systemd.git
        $ git fetch --all

- Locate your pristine copy from `debian/pristine-tar`. See avaliables using:

        $ git log --oneline --decorate debian/pristine-tar

- Mark pristine-tar commit and ubuntu version (here for ubuntu systemd 219)

        $ git logg 81d67ad

        $ git branch pristine-tar 81d67ad

- Mark desired commit for new version (usually master^ from this repo). Parent is to avoid this README.md file.

        $ git checkout -b ubuntu-vivid master^

- Create source package file (it will be created on parent dir (../), that's why sandbox)

        $ git-buildpackage -S -us -uc -nc

- Generate package. You need to avoid execute testsuite

        $ cd ..
        $ DEB_BUILD_OPTIONS=nocheck sbuild -d vivid -j4 systemd_*.dsc

- Install all .deb generated

    You need to clean this files prior to install

        $ rm /usr/share/doc/libsystemd0/changelog.Debian.gz
        $ rm /usr/share/doc/libudev1/changelog.Debian.gz
        $ sudo dpkg -i *.deb

## Easy install (outdated)

Install the package compiled with

     $ sudo add-apt-repository ppa:thopiekar/miraclecast
     $ sudo apt-get update
     $ sudo apt-get upgrade
     $ sudo apt-get install miraclecast .

## Original repos

- freedesktop:

  - web: http://www.freedesktop.org/software/systemd/
  - vcs: git://anongit.freedesktop.org/systemd/systemd

- debian

  - web: https://anonscm.debian.org/cgit/pkg-systemd/systemd.git
  - vcs: git://anonscm.debian.org/pkg-systemd/systemd.git

- ubuntu
  - web: https://launchpad.net/ubuntu/+source/systemd/&lt;version&gt;
  - vcs: lp:ubuntu/systemd
