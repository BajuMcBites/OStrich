

.global clean_dcache_range
clean_dcache_range:
1:  dc  cvac, x0
    add x0, x0, #64
    subs x1, x1, #64
    b.gt 1b
    dsb sy
    ret

.global invalidate_dcache_range
invalidate_dcache_range:
1:  dc  ivac, x0
    add x0, x0, #64
    subs x1, x1, #64
    b.gt 1b
    dsb sy
    ret

.global clean_and_invalidate_dcache_range
clean_and_invalidate_dcache_range:
1:  dc  civac, x0
    add x0, x0, #64
    subs x1, x1, #64
    b.gt 1b
    dsb sy
    ret

.global clean_dcache_line
clean_dcache_line:
    dc  cvac, x0
    dsb sy
    ret

.global invalidate_dcache_line
invalidate_dcache_line:
    dc  ivac, x0
    dsb sy
    ret

.global clean_and_invalidate_dcache_line
clean_and_invalidate_dcache_line:
    dc  civac, x0
    dsb sy
    ret

.global dmb
dmb:
    dmb sy
    ret

.global dsb
dsb:
    dsb sy
    ret
