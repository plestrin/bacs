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
| ADC | 2 | Modular with carry. Incorrect: should take an extra argument Do not normalize. |
| ADD | > 1 | Modular addition. |
| AND | > 1 | Bitwise AND. |
| CMOV | 2 | Conditional move. Incorrect: should take an extra argument. Do not normalize. |
| DIVQ | 2 | Division return quotient. |
| DIVR | 2 | Division return the remainder. |
| IDIV | 2 | Signed division returned both quotient and remainder. Might be incorrect in some case. |
| IMUL | > 1 | Signed modular multiplication. |
| MOVZX | 1 | Increase the size of variable and pad with zeroes.|
| MUL | > 1 | Modular multiplication. |
| NEG | 1 | Two's complement.|
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