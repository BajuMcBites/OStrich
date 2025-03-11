.global invalidate_icache_range
invalidate_icache_range:
1:  ic ivau, x0
    add x0, x0, #64
    subs x1, x1, #64
    b.gt 1b
    isb
    ret

.global invalidate_icache_all
invalidate_icache_all:
    ic iallu
    isb
    ret

.global isb
isb:
    isb
    ret
