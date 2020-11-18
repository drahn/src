.include <bsd.own.mk>

# Force Target & Machine Arch for Cross-Compile
CFLAGS+="--target=riscv64-unknown-openbsd6.6 -march=rv64gc"
# No relax during linking
LDADD+="-Wl,-z,notext"

CERROR=cerror.S

