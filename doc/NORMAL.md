<style media="screen" type="text/css">
	table {
		color:#333333;
		border-width: 1px;
		border-color: #666666;
		border-collapse: collapse;
	}
	table th {
		border-width: 1px;
		padding: 8px;
		border-style: solid;
		border-color: #666666;
		background-color: #dedede;
	}
	table td {
		border-width: 1px;
		padding: 8px;
		border-style: solid;
		border-color: #666666;
		background-color: #ffffff;
	}
</style>

# Normalization Guide

## List of Operations

| Operation | Arity | Description |
| --------- | ----- | ----------- |
| ADC | 2 | Modular addition with carry. Incorrect: should take an extra argument for the carry. Do not normalize. |
| ADD | > 1 | Modular addition. |
| AND | > 1 | Bitwise AND. |
| CMOV | 2 | Conditional move. Incorrect: should take an extra argument for FLAGS. Do not normalize. |
| DIVQ | 2 | Division, returns the quotient. |
| DIVR | 2 | Division, returns the remainder. |
| IDIVQ | 2 | Signed division, returns the quotient. |
| IDIVR | 2 | Signed division, returns the remainder. |
| IMUL | > 1 | Signed modular multiplication. |
| MOVZX | 1 | Increase the size of variable and pad with zeroes.|
| MUL | > 1 | Modular multiplication. |
| NEG | 1 | Two's complement.|
| NOT | 1 | Bitwise NOT. |
| OR | > 1 | Bitwise OR. |
| PART1_8 | 1 | Takes the 8 least significant bits. |
| PART2_8 | 1 | Takes bit segment [8:15] starting from the least significant bits. |
| PART1_16 | 1 | Takes the 16 least significant bits. |
| ROL | 2 | Rotate to the left. |
| ROR | 2 | Rotate to the right. |
| SHL | 2 | Shift to the left. |
| SHLD | 3 | Shift to the left and shift in from the right bits from the third operand. |
| SHR | 2 | Shift to the right. |
| SHRD | 3 | Shift to the right and shift in from the left bits from the third operand. |
| SUB | 2 | Modular subtraction. |
| XOR | > 1 | Bitwise XOR. |

## Miscellaneous Rewrite Rules

### ADD:
* Complex rules based on merging: use diff value, final flag and cst*

### AND:
* If there is a constant operand and if it does not modify the set of reachable values -> remove it from the operand list;
* If an operand is a AND and both have a constant operand -> merge (edge copy).
*This last rule should be moved to the MERGE generic pattern*

### MOVZX 
* If the operand is PART1_8, PART1_16 and the size of its operand is equal to its size -> replace both instructions by AND;
* If the operand is PART2_8 and the size of its operand is equal to its size -> replace both instructions by SHR and AND.
*Do we really need this rule (it seems to me that it is redundant with what is done in irVaraiableSize)?*

### MUL
* If there are only two operands and one is a constant that is equal to a power of 2 -> replace by SHL.

## OR
* If several operands are equal -> keep only on of them ;
* If an operand is a OR with a single outgoing edge -> merge.
*This last rule should be moved to the MERGE generic pattern*

### PART1_8
* If the operand is MOVZX -> replace the current node by MOVZ operand ;
* If the operand is PART1_16 -> replace in the operand list PART1_16 by its operand.
*Plus something that is more related with constant propagation than with anything else.*

### PART2_8
* If the operand is PART1_16 -> replace in the operand list PART1_16 by its operand.
*Plus something that is more related with constant propagation than with anything else.*

### ROL
* If the disp operand is a constant -> replace by ROR (adjust the value of the disp operand).

### SHL
* If the direct operand is SHR or SHL and if the disp operand is a constant -> replace by a single shift with a masking operation if necessary.

### SHLD
* If both the first and the third operand are equal -> replace by ROL.

### SHR
* If the direct operand is SHR or SHL and if the disp operand is a constant -> replace by a single shift with a masking operation if necessary.

### SHRD
* If both the first and the third operand are equal -> replace by ROL.


### SUB
* If the sub operand is an immediate -> replace by and ADD.

### XOR
* If two operands are equal -> remove both (if the operand count fall to zero, replace by a constant);
* If there are two operand and one of them is a constant equal to 0xff..ff -> replace by NOT.
* Complex rules based on merging: use diff value and final flag*


## Topological Order

| Rules | Order | Remarks |
| ----- | ----- | ------- |
| Constant Propagation | DST -> SRC | Safe node iteration (only deletes operands). |
| Dead Code Removal | DST -> SRC | Mandatory to delete everything in a single pass. |
| Misc Rewriting | DST -> SRC | Safe node iteration. |



