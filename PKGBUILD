pkgname=rofi-qalc-git
pkgver=1.0.0
pkgrel=0
pkgdesc='Rofi calculator mode using libqalculate'
arch=('x86_64')
url='https://github.com/svenvvv/rofi-qalc'
license=('GPL-2.0-only')
depends=('rofi' 'libqalculate')
makedepends=('meson' 'git')
source=('git+https://github.com/svenvvv/rofi-qalc')
sha512sums=('SKIP')

_gitname='rofi-qalc'

prepare() {
  cd "$srcdir/$_gitname"
  meson setup build \
    --buildtype=release \
    --optimization=3 \
    -Dstrip=true \
    -Dlibdir=lib/rofi \
    -Dprefix=/usr
}

build() {
  cd "$srcdir/$_gitname"
  meson compile -C build
}

package() {
  cd "$srcdir/$_gitname"
  meson install --destdir "$pkgdir/" -C build
  install -Dm644 LICENSE "$pkgdir"/usr/share/licenses/$pkgname/LICENSE
}
