MZ       ����          @       www.trimcore.cz             @   PE  L '��e        � #&                      @                    P     �c    �                           �!  d    @  �                          X!                                                 P                           .text   F                          `.rdata  �                        @  @.data       0                      @  �.rsrc   �   @                    @  @                                        �  h0@ h0@ h0@ �H @ h0@ jj hP @ h  �� @ �%0@ ����  jY�0@ �@  j
Y�8  �=0@ 
r	�   ��u3ҹ� @ �   ��u
�� @ ��  �� @ ��  ��[  3ҹ� @ �   ��u3ҹ� @ �   ��tj Y��  �^  �>  �� @ �� @ �[   j]Y�  jY�  j
Y�  j �8 @ �Sh!@ 2��4 @ ��t$h!@ P�  @ ��th4!@ �Ѕ�t	���J  �Ê�[�U��0@ ��  V���tJ�U��E�   R������Rj j QP�  @ ��u(�U���v��t
����  �U��ꍍ����J�c  ��2�^��U���h���M�V3�Ij
^����0���u�W�}�3�+���~
�U ��   �WQ�E�P�m  �U��׋�_��u�E�+�tf�D5�f�Du�F;�u��M���   ^��U��QQ�0@ ��
u�=0@  u���  �0@ �e���j.Y�X  �0@ �R���j.Y�E  ��  �0@ �:����E��E�   P3ҍE�PRRhT!@ �50@ �U�� @ ��uj.Y�  �M��������U��Qj��$ @ � 0@ ��uj�8 @ P�0 @ �� t%��t ��u�E�P�5 0@ � @ �0@  ��u�0@ ��U���  �=0@  SVW���tT��td�   j j P��;�������PB�SWj j �, @ ��~<j �M�QP������P�5 0@ � @ �<_�   +�u��j �E�PVW�5 0@ � @ _^[�Ë�V�rf���f��u�+���^�_���U��QQ�=0@  �E�j Pj�E�f�M�P�5 0@ t� @ ��� @ ��U����=0@  u�E�P�5 0@ � @ ��t�E���2��À=0@  u&�0@ �����
�:0@ t��P�5 0@ �( @ À=0@  u�0@ P�5 0@ �( @ �����%@ @                                                                                                                                                                                                                                                                                                                                                                                                                                                           <#  `#  P#      �"  �"  d"  �"  �"  �"  �"  
#   #  �"  �"      �#      @"      SOFTWARE\Microsoft\Windows NT\CurrentVersion    P r o d u c t N a m e   W i n d o w s     [ V e r s i o n       D i s p l a y V e r s i o n     R e l e a s e I d       C S D V e r s i o n     WINBRAND    BrandingFormatString    % W I N D O W S _ L O N G %     UBR     '��e                       � �                8"          Z"  H    "          .#     �!          t#      0"          �#  @                       <#  `#  P#      �"  �"  d"  �"  �"  �"  �"  
#   #  �"  �"      �#      @"      >RtlGetNtVersionNumbers  ntdll.dll GetConsoleScreenBufferInfo  0SetConsoleTextAttribute �GetStdHandle  GWriteFile GetConsoleMode  �LoadLibraryA  FWriteConsoleW �GetProcAddress  zExitProcess 3WideCharToMultiByte mGetFileType KERNEL32.dll  �RegQueryValueExW  �RegOpenKeyExA �RegQueryValueExA  ADVAPI32.dll   memcpy  msvcrt.dll                                                                                                                               �   8  �                  P  �                  h  �               	  �                  	  �   �@  H          �C  �          H4   V S _ V E R S I O N _ I N F O     ���      �      �   ?                          �   S t r i n g F i l e I n f o   �   0 4 0 9 0 4 B 0   .   I n t e r n a l N a m e   w i n v e r     >   O r i g i n a l F i l e n a m e   w i n v e r . e x e     T   C o m p a n y N a m e     T R I M   C O R E   S O F T W A R E   s . r . o .   f !  L e g a l C o p y r i g h t   �   2 0 2 4   T R I M   C O R E   S O F T W A R E   s . r . o .     4 
  F i l e V e r s i o n     0 . 2 . 0 . 1 6 4   j !  F i l e D e s c r i p t i o n     W i n d o w s   V e r s i o n   a n d   I n f o   P r i n t e r     .   P r o d u c t N a m e     W i n V e r     0   P r o d u c t V e r s i o n   0 . 2 . 0   >   U R L     h t t p : / / w w w . t r i m c o r e . c z     D    V a r F i l e I n f o     $    T r a n s l a t i o n     	�﻿<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<assembly
	xmlns="urn:schemas-microsoft-com:asm.v1"
	manifestVersion="1.0">
<assemblyIdentity
	name="TRIMCORE.WinVer.winver.exe"
	processorArchitecture="x86"
	version="0.2.0.164"
	type="win32" />
<description>Windows Version and Info Printer</description>
<application xmlns="urn:schemas-microsoft-com:asm.v3">
	<windowsSettings>
		<activeCodePage xmlns="http://schemas.microsoft.com/SMI/2019/WindowsSettings">UTF-8</activeCodePage>
	</windowsSettings>
</application>
<compatibility xmlns="urn:schemas-microsoft-com:compatibility.v1">
	<application>
		<supportedOS Id="{e2011457-1546-43c5-a5fe-008deee3d3f0}" />
		<supportedOS Id="{35138b9a-5d96-4fbd-8e2d-a2440225f93a}" />
		<supportedOS Id="{4a2f28e3-53b9-4441-ba9c-d69d4a4a6e38}" />
		<supportedOS Id="{1f676c76-80e1-4239-95bb-83d0f6d0da78}" />
		<supportedOS Id="{8e0f7a12-bfb3-4fe8-b9a5-48fd50a15a9a}" />
	</application>
</compatibility>
</assembly>
                                                           