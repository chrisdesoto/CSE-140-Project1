# Short test case for your project.
#
# Note that this is by no means a comprehensive test!
#

# Testing finished andi command

		.text	
_start:		
		addiu	$8, $0, 15
		andi	$9, $8, 3
		andi	$10, $8, 8
test:		addiu	$8, $8, 10		