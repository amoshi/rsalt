# rsalt
remote saltstack execution tool

## requirements
- cmake, c compiler, conan needs for build
- To install conan use: pip3 install conan
- Works via rest\_wsgi salt-api interface or other implementation: https://github.com/amoshi/salt-yapi
- multi-saltstack configuration (routed by contexts in /etc/rsalt.conf ini file).

## build:
```
pip3 install conan
conan install . --build=missing
make install
```

## config file (/etc/rsalt.conf) examples:
```
[default]
saltapi=http://saltstack.example.com:8080/
eauth=pam
username=saltuser
password=saltpassword

[othersalt]
saltapi=http://basicuser:basicpassword@saltstack2.example.com:8080/
eauth=ldap
username=saltuser
password=saltpassword
```

## configuration from env (for CI):
```
RSALT_SALTAPI
RSALT_EAUTH
RSALT_USERNAME
RSALT_PASSWORD
```

## examples
### default context
```
rsalt srv1.example.com state.highstate
rsalt -G os:FreeBSD state.sls freebsd.system
rsalt -L 'srv1.example.com,srv2.example.com' grains.get cpuarch
```

### select context
```
rsalt --context othersalt srv?.example.com grains.get cpuarch
```

### match output (support extend POSIX regex)
```
rsalt --context othersalt srv?.example.com state.sls redis --showids
rsalt --context othersalt srv?.example.com state.sls redis --match 'file__-/etc/redis.conf__-/etc/redis.conf__-managed.changes'
rsalt --context othersalt srv?.example.com state.sls redis --match 'file__-/etc/redis.conf__-/etc/redis.conf__-managed.changes|file__-/etc/redis.conf__-/etc/redis.conf__-managed.comments'
```

### for debugging
```
rsalt --context othersalt srv?.example.com state.sls redis --dry-run
```
