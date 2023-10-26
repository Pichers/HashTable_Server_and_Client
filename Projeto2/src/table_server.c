//Retradução do codigo do stor
// undefined8
// main(int param_1,long param_2,undefined8 param_3,char *param_4,long **param_5,long **param_6)

// {
//   int iVar1;
//   undefined8 uVar2;
//   char **ppcVar3;
//   size_t sVar4;
//   char *pcVar5;
//   char *pcVar6;
//   void *pvVar7;
//   int *piVar8;
//   void **ppvVar9;
//   undefined8 extraout_RDX;
//   undefined8 extraout_RDX_00;
//   undefined8 extraout_RDX_01;
//   undefined8 extraout_RDX_02;
//   undefined8 extraout_RDX_03;
//   undefined8 extraout_RDX_04;
//   undefined *puVar10;
//   long in_FS_OFFSET;
//   int local_4a4;
//   int local_4a0;
//   int local_49c;
//   int local_498;
//   char local_428 [1032];
//   long local_20;
  
//   local_20 = *(long *)(in_FS_OFFSET + 0x28);
//   if (param_1 == 2) {
//     ppcVar3 = rtable_connect(*(char **)(param_2 + 8));
//     if (ppcVar3 != (char **)0x0) {
//       signal(0xd,(__sighandler_t)0x1);
// LAB_00102542:
//       do {
//         while( true ) {
//           while( true ) {
//             while( true ) {
//               do {
//                 printf("Command: ");
//                 fgets(local_428,0x400,stdin);
//                 sVar4 = strlen(local_428);
//               } while (sVar4 == 1);
//               pcVar5 = strtok(local_428," \n");
//               iVar1 = strcmp(pcVar5,"put");
//               if ((iVar1 != 0) && (iVar1 = strcmp(pcVar5,"p"), iVar1 != 0)) break;
//               pcVar5 = strtok((char *)0x0," \n");
//               pcVar6 = strtok((char *)0x0,"\n");
//               if ((pcVar5 == (char *)0x0) || (pcVar6 == (char *)0x0)) {
//                 puts("Invalid arguments. Usage: put <key> <value>");
//               }
//               else {
//                 sVar4 = strlen(pcVar6);
//                 pvVar7 = malloc(sVar4);
//                 sVar4 = strlen(pcVar6);
//                 param_4 = pcVar6;
//                 memcpy(pvVar7,pcVar6,sVar4);
//                 sVar4 = strlen(pcVar6);
//                 piVar8 = data_create((int)sVar4,(long)pvVar7);
//                 pcVar5 = strdup(pcVar5);
//                 ppvVar9 = (void **)entry_create((long)pcVar5,(long)piVar8);
//                 uVar2 = rtable_put((long)ppcVar3,ppvVar9,ppvVar9,param_4,param_5,param_6);
//                 if ((int)uVar2 == -1) {
//                   puts("Error in rtable_put!");
//                 }
//                 entry_destroy(ppvVar9);
//               }
//             }
//             iVar1 = strcmp(pcVar5,"get");
//             if ((iVar1 != 0) && (iVar1 = strcmp(pcVar5,"g"), iVar1 != 0)) break;
//             pcVar5 = strtok((char *)0x0," \n");
//             if (pcVar5 == (char *)0x0) {
//               puts("Invalid arguments. Usage: get <key>");
//             }
//             else {
//               piVar8 = rtable_get((long)ppcVar3,pcVar5,pcVar5,param_4,param_5,param_6);
//               if (piVar8 == (int *)0x0) {
//                 puts("Error in rtable_get or key not found!");
//               }
//               else {
//                 for (local_4a4 = 0; local_4a4 < *piVar8; local_4a4 = local_4a4 + 1) {
//                   putchar((int)*(char *)((long)local_4a4 + *(long *)(piVar8 + 2)));
//                 }
//                 putchar(10);
//                 data_destroy(piVar8);
//               }
//             }
//           }
//           iVar1 = strcmp(pcVar5,"del");
//           if ((iVar1 != 0) && (iVar1 = strcmp(pcVar5,"d"), iVar1 != 0)) break;
//           pcVar5 = strtok((char *)0x0," \n");
//           if (pcVar5 == (char *)0x0) {
//             puts("Invalid arguments. Usage: del <key>");
//           }
//           else {
//             uVar2 = rtable_del((long)ppcVar3,pcVar5,pcVar5,param_4,param_5,param_6);
//             if ((int)uVar2 == -1) {
//               puts("Error in rtable_del or key not found!");
//             }
//             else {
//               puts("Entry removed");
//             }
//           }
//         }
//         puVar10 = &DAT_0010a154;
//         iVar1 = strcmp(pcVar5,"size");
//         uVar2 = extraout_RDX;
//         if (iVar1 != 0) {
//           puVar10 = &DAT_0010a159;
//           iVar1 = strcmp(pcVar5,"s");
//           uVar2 = extraout_RDX_00;
//           if (iVar1 != 0) {
//             pcVar6 = "getkeys";
//             iVar1 = strcmp(pcVar5,"getkeys");
//             uVar2 = extraout_RDX_01;
//             if (iVar1 != 0) {
//               pcVar6 = "k";
//               iVar1 = strcmp(pcVar5,"k");
//               uVar2 = extraout_RDX_02;
//               if (iVar1 != 0) {
//                 pcVar6 = "gettable";
//                 iVar1 = strcmp(pcVar5,"gettable");
//                 uVar2 = extraout_RDX_03;
//                 if (iVar1 != 0) {
//                   pcVar6 = "t";
//                   iVar1 = strcmp(pcVar5,"t");
//                   uVar2 = extraout_RDX_04;
//                   if (iVar1 != 0) {
//                     iVar1 = strcmp(pcVar5,"quit");
//                     if ((iVar1 == 0) || (iVar1 = strcmp(pcVar5,"q"), iVar1 == 0)) goto LAB_00102c16;
//                     puts(
//                         "Invalid command.\nUsage: p[ut] <key> <value> | g[et] <key> | d[el] <key> | s[ize] | [get]k[eys] | [get]t[able] | q[uit]"
//                         );
//                     goto LAB_00102542;
//                   }
//                 }
//                 pvVar7 = rtable_get_table((long)ppcVar3,pcVar6,uVar2,param_4,param_5,param_6);
//                 if (pvVar7 == (void *)0x0) {
//                   puts("Error in rtable_get_table");
//                 }
//                 else {
//                   local_49c = 0;
//                   while (*(long *)((long)pvVar7 + (long)local_49c * 8) != 0) {
//                     printf("%s :: ",**(char ***)((long)pvVar7 + (long)local_49c * 8));
//                     for (local_498 = 0;
//                         local_498 < **(int **)(*(long *)((long)pvVar7 + (long)local_49c * 8) + 8);
//                         local_498 = local_498 + 1) {
//                       putchar((int)*(char *)((long)local_498 +
//                                             *(long *)(*(long *)(*(long *)((long)pvVar7 +
//                                                                          (long)local_49c * 8) + 8) +
//                                                      8)));
//                     }
//                     putchar(10);
//                     local_49c = local_49c + 1;
//                   }
//                   rtable_free_entries(pvVar7);
//                 }
//                 goto LAB_00102542;
//               }
//             }
//             pvVar7 = rtable_get_keys((long)ppcVar3,pcVar6,uVar2,param_4,param_5,param_6);
//             if (pvVar7 == (void *)0x0) {
//               puts("Error in rtable_getkeys");
//             }
//             else {
//               local_4a0 = 0;
//               while (*(long *)((long)pvVar7 + (long)local_4a0 * 8) != 0) {
//                 puts(*(char **)((long)pvVar7 + (long)local_4a0 * 8));
//                 local_4a0 = local_4a0 + 1;
//               }
//               rtable_free_keys(pvVar7);
//             }
//             goto LAB_00102542;
//           }
//         }
//         iVar1 = rtable_size((long)ppcVar3,puVar10,uVar2,param_4,param_5,param_6);
//         if (iVar1 == -1) {
//           puts("Error in rtable_size");
//         }
//         else {
//           printf("Table size: %d\n",iVar1);
//         }
//       } while( true );
//     }
//     uVar2 = 0xffffffff;
//   }
//   else {
//     puts("Invalid args!");
//     puts("Usage: table-client <server>:<port>");
//     uVar2 = 0xffffffff;
//   }
// LAB_00102c4f:
//   if (local_20 == *(long *)(in_FS_OFFSET + 0x28)) {
//     return uVar2;
//   }
//                     /* WARNING: Subroutine does not return */
//   __stack_chk_fail();
// LAB_00102c16:
//   rtable_disconnect(ppcVar3);
//   puts("Bye, bye!");
//   uVar2 = 0;
//   goto LAB_00102c4f;
// }

