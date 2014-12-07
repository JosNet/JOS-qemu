#!/usr/bin/env perl

open(my $fh, '>', "kern/entrypgdir.c")
    or die "Couldn't generate entry pgdir for qemu!";
print $fh (<<EOF

#include <inc/mmu.h>
#include <inc/memlayout.h>

pte_t entry_pgtable_a[0x400];
pte_t entry_pgtable_b[0x400];

// The entry.S page directory maps the first 4MB of physical memory
// starting at virtual address KERNBASE (that is, it maps virtual
// addresses [KERNBASE, KERNBASE+4MB) to physical addresses [0, 4MB)).
// We choose 4MB because that's how much we can map with one page
// table and it's enough to get us through early boot.  We also map
// virtual addresses [0, 4MB) to physical addresses [0, 4MB); this
// region is critical for a few instructions in entry.S and then we
// never use it again.
//
// Page directories (and page tables), must start on a page boundary,
// hence the "__aligned__" attribute.  Also, because of restrictions
// related to linking and static initializers, we use "x + PTE_P"
// here, rather than the more standard "x | PTE_P".  Everywhere else
// you should use "|" to combine flags.

__attribute__((__aligned__(PGSIZE)))
pde_t entry_pgdir[NPDENTRIES] = {
	// Map VA's [0, 4MB) to PA's [0, 4MB)
	[0]
		= ((uintptr_t)entry_pgtable_a - KERNBASE) + PTE_P,
	[1]
		= ((uintptr_t)entry_pgtable_b - KERNBASE) + PTE_P,
	// Map VA's [KERNBASE, KERNBASE+4MB) to PA's [0, 4MB)
	[KERNBASE>>PDXSHIFT]
		= ((uintptr_t)entry_pgtable_a - KERNBASE) + PTE_P + PTE_W,
    [(KERNBASE>>PDXSHIFT)+1]
		= ((uintptr_t)entry_pgtable_b - KERNBASE) + PTE_P + PTE_W
};

EOF
);

print $fh (<<EOF
// Entry 0 of the page table maps to physical page 0, entry 1 to
// physical page 1, etc.
__attribute__((__aligned__(PGSIZE)))
pte_t entry_pgtable_a[NPTENTRIES] = {
EOF
);

for my $i (0..(0x399)) {
    my $paddr = sprintf("0x%04x0000", $i);
    print $fh qq($paddr | PTE_P | PTE_W);
    if($i != 0x1000) {
        print $fh qq(,\n);
    }
}

print $fh (<<EOF
};

EOF
);

print $fh (<<EOF
// Entry 0 of the page table maps to physical page 0, entry 1 to
// physical page 1, etc.
__attribute__((__aligned__(PGSIZE)))
pte_t entry_pgtable_b[NPTENTRIES] = {
EOF
);

for my $i (0x400..(0x799)) {
    my $paddr = sprintf("0x%04x0000", $i);
    print $fh qq($paddr | PTE_P | PTE_W);
    if($i != 0x1000) {
        print $fh qq(,\n);
    }
}

print $fh (<<EOF
};

EOF
);


close($fh);
1;
