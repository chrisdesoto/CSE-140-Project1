# Short test case for your project.
#
# Note that this is by no means a comprehensive test!
#

# If $zero can be written to, code
# results in an infinite loop.

		.text	
_start:		
		addiu	$8, $0, 0
test:	addiu 	$0, $0, 10
		bne	$0, $8 test			