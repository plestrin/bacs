if (argc == 2 && !strcmp(argv[1], "-v")){
	printf("NAME           | VALUE         | DESC                                 \n");
	printf("---------------|---------------|--------------------------------------\n");

	#if ENC_UNROLL == FULL
	printf("ENC_UNROLL     | FULL          |                                      \n");
	#elif ENC_UNROLL == PARTIAL
	printf("ENC_UNROLL     | PARTIAL       |                                      \n");
	#else
	printf("ENC_UNROLL     | NONE          |                                      \n");
	#endif

	#if DEC_UNROLL == FULL
	printf("DEC_UNROLL     | FULL          |                                      \n");
	#elif DEC_UNROLL  == PARTIAL
	printf("DEC_UNROLL     | PARTIAL       |                                      \n");
	#else
	printf("DEC_UNROLL     | NONE          |                                      \n");
	#endif

	#ifdef ENC_KS_UNROLL
	printf("ENC_KS_UNROLL  | --def--       |                                      \n");
	#else
	printf("ENC_KS_UNROLL  | --undef--     |                                      \n");
	#endif

	#ifdef DEC_KS_UNROLL
	printf("DEC_KS_UNROLL  | --def--       |                                      \n");
	#else
	printf("DEC_KS_UNROLL  | --undef--     |                                      \n");
	#endif

	#ifdef FIXED_TABLES
	printf("FIXED_TABLES   | --def--       | The tables used by the code are compiled statically into the binary file\n");
	#else
	printf("FIXED_TABLES   | --undef--     | The subroutine aes_init() must be called to compute them before the code is first used\n");
	#endif

	#if ENC_ROUND == FOUR_TABLES
	printf("ENC_ROUND      | FOUR_TABLES   | Set tables for the normal encryption round\n");
	#elif ENC_ROUND == ONE_TABLE
	printf("ENC_ROUND      | ONE_TABLE     | Set tables for the normal encryption round\n");
	#else
	printf("ENC_ROUND      | NO_TABLES     | Set tables for the normal encryption round\n");
	#endif

	#if LAST_ENC_ROUND == FOUR_TABLES
	printf("LAST_ENC_ROUND | FOUR_TABLES   | Set tables for the last encryption round\n");
	#elif LAST_ENC_ROUND == ONE_TABLE
	printf("LAST_ENC_ROUND | ONE_TABLE     | Set tables for the last encryption round\n");
	#else
	printf("LAST_ENC_ROUND | NO_TABLES     | Set tables for the last encryption round\n");
	#endif

	#if DEC_ROUND == FOUR_TABLES
	printf("DEC_ROUND      | FOUR_TABLES   | Set tables for the normal decryption round\n");
	#elif DEC_ROUND == ONE_TABLE
	printf("DEC_ROUND      | ONE_TABLE     | Set tables for the normal decryption round\n");
	#else
	printf("DEC_ROUND      | NO_TABLES     | Set tables for the normal decryption round\n");
	#endif

	#if LAST_DEC_ROUND == FOUR_TABLES
	printf("LAST_DEC_ROUND | FOUR_TABLES   | Set tables for the last decryption round\n");
	#elif LAST_DEC_ROUND == ONE_TABLE
	printf("LAST_DEC_ROUND | ONE_TABLE     | Set tables for the last decryption round\n");
	#else
	printf("LAST_DEC_ROUND | NO_TABLES     | Set tables for the last decryption round\n");
	#endif

	#if KEY_SCHED == FOUR_TABLES
	printf("KEY_SCHED      | FOUR_TABLES   | Set tables for the key schedule\n");
	#elif KEY_SCHED == ONE_TABLE
	printf("KEY_SCHED      | ONE_TABLE     | Set tables for the key schedule\n");
	#else
	printf("KEY_SCHED      | NO_TABLES     | Set tables for the key schedule\n");
	#endif

	printf("\n");
}
