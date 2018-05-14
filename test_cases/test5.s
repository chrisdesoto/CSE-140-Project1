# Short test case for your project.
#
# Note that this is by no means a comprehensive test!
#

# Sums digits from 100 -> 0 and
# tests the lw & sw instructions
# at the first available memory
# address.

		.text	
_start:
		addiu	$t0,$0,100
		addiu	$t1,$0,0
		addiu	$t2,$0,0
loop:		addu	$t1,$t1,$t0
		addiu	$t0,$t0,-1
		bne	$t0,$0,loop
		lui	$t0,0x0040
		addiu	$t0,$t0,0x1000
		sw	$t1,0($t0)
		lw	$t2,0($t0)
		sll	$t2,$t2,2