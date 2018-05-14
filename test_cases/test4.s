# Short test case for your project.
#
# Note that this is by no means a comprehensive test!
#

# Accessing valid memory address then
# runs an unsupported instruction and
# terminates

		.text
		addiu	$a0,$0,3
		addiu	$a1,$0,2
		lui	$t0,64
		addiu	$t0,$t0,8192
		sw	$a1 0($t0)	
		lw	$a2 0($t0)
		addiu	$a1,$a1,2
		addi	$t0,$t0,-8192	# Unsupported instruction. Exit(0)
		sw	$a1 0($t0)