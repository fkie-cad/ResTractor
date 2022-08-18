set(HEADER_PARSER_SRC
	src/art/ArtFileHeader.h
	src/art/ArtHeaderOffsets.h
	src/art/ArtHeaderParser.h
	src/art/ArtHeaderPrinter.h
	src/dex/DexFileHeader.h
	src/dex/DexHeaderOffsets.h
	src/dex/DexHeaderParser.h
	src/dex/DexHeaderPrinter.h
	src/elf/ElfInstructionSetArchitecture.h
	src/elf/ElfHeaderOffsets.h
	src/elf/ElfFileHeader.h
	src/elf/ElfFileType.h
	src/elf/ElfSectionHeader.h
	src/elf/ElfHeaderParser.h
	src/elf/ElfHeaderPrinter.h
	src/elf/ElfOSAbiIdentification.h
	src/elf/ElfProgramHeader.h
	src/elf/ElfProgramHeaderFlags.h
	src/elf/ElfProgramHeaderTypes.h
	src/elf/ElfSectionHeaderTypes.h
	src/jar/JarHeaderOffsets.h
	src/jar/JarHeaderParser.h
	src/jar/JarHeaderPrinter.h
	src/java/JavaClassHeaderParser.h
	src/macho/MachOFileHeader.h
	src/macho/MachOCPUTypes.h
	src/macho/MachOHeaderOffsets.h
	src/macho/MachOHeaderPrinter.h
	src/macho/MachOHeaderParser.h
	src/msi/MsiHeader.h
	src/msi/MsiHeaderParser.h
	src/msi/MsiHeaderPrinter.h
	src/pe/PECharacteristics.h
	src/pe/PECertificateHandler.h
	src/pe/PEImageDirectoryParser.h
	src/pe/PEMachineTypes.h
	src/pe/PEHeaderOffsets.h
	src/pe/PEOptionalHeaderSignature.h
	src/pe/PEHeaderParser.h
	src/pe/PEHeaderPrinter.h
	src/pe/PEHeaderSectionNameResolution.h
	src/pe/PESectionCharacteristics.h
	src/pe/PESymbolTable.h
	src/pe/PEWindowsSubsystem.h
	src/utils/blockio.h
	src/utils/common_fileio.h
	src/utils/Converter.h
	src/utils/Helper.h
	src/zip/ZipHeader.h
	src/zip/ZipHeaderOffsets.h
	src/zip/ZipHeaderParser.h
	src/zip/ZipHeaderPrinter.h
	src/ArchitectureInfo.h
	src/Globals.h
	src/HeaderData.h
	src/PEHeaderData.h
	src/headerDataHandler.h
	src/stringPool.h
	src/parser.h
	)

set(HEADER_PARSER_LIB_FILES
	src/headerParserLib.c
	src/headerParserLib.h
	src/headerParserLibPE.h
	${HEADER_PARSER_SRC}
	)