# Password Store Git Helper

This is a [git] credential helper that uses [pass] as its backend. It
assumes that the host is present in the [pass] entry name, and that
the file itself contains either a [user:], [username:], or [login:]
line.

## Installation

```sh
make
sudo make install
```

See the `Makefile` for details.

[git]: https://git-scm.com/
[pass]: http://www.passwordstore.org/ "pass - the standard unix password manager"
