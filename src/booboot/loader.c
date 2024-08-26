#include "loader.h"
#include "file.h"
#include "mem.h"

#include <handover/handover.h>
#include <logging>
#include <string.h>

void *_Nullable _ehdr = NULL;
size_t _ehdr_size = 0;

uintptr_t load_binary(char const *_Nonnull path)
{
    char *_Nonnull hdr = efi_read_file(path, &_ehdr_size);
    _ehdr = hdr;

    char magic[4] = {hdr[EI_MAG0], hdr[EI_MAG1], hdr[EI_MAG2], hdr[EI_MAG3]};

    if (memcmp(magic, ELFMAG, SELFMAG) != 0)
    {
        error$("%s is not a valid ELF file", path);
        for (;;)
        {
            __asm__("hlt");
        }
    }

    bool is_64bits = hdr[EI_CLASS] == ELFCLASS64;

    debug$("loading binary %s...", path);

    if (is_64bits)
    {
        Elf64_Ehdr *ehdr = (Elf64_Ehdr *)hdr;
        for (size_t i = 0; i < ehdr->e_phnum; i++)
        {
            Elf64_Phdr *phdr = (Elf64_Phdr *)(hdr + ehdr->e_phoff + i * ehdr->e_phentsize);
            if (phdr->p_type != PT_LOAD)
            {
                continue;
            }

            debug$("loading segment %llx", phdr->p_vaddr);

            uintptr_t page = paging_alloc(align_up$(phdr->p_memsz, PAGE_SIZE) / PAGE_SIZE);

            if (pml4() == NULL)
            {
                error$("paging not initialized, what are those developers doing?");
                for (;;)
                {
                    __asm__("hlt");
                }
            }

            paging_map_range(pml4(), phdr->p_vaddr, page, align_up$(phdr->p_memsz, PAGE_SIZE), PAGE_PRESENT | PAGE_WRITABLE);

            memcpy((void *)page, (void *)(hdr + phdr->p_offset), phdr->p_filesz);
            memcpy((void *)(page + phdr->p_filesz), 0, phdr->p_memsz - phdr->p_filesz);
        }

        debug$("entry: %llx", ehdr->e_entry);
        return ehdr->e_entry;
    }
    else
    {
        warn$("32-bits support is in the todo list");
        for (;;)
        {
            __asm__("hlt");
        }

        __builtin_unreachable();
    }

    __builtin_unreachable();
}

void *_Nullable ehdr(void)
{
    return _ehdr;
}

size_t ehdr_len(void)
{
    return _ehdr_size;
}

void *_Nonnull load_section(char const *_Nonnull name)
{
    bool found = false;
    if (_ehdr == NULL)
    {
        error$("no binary loaded");
        for (;;)
        {
            __asm__("hlt");
        }
    }

    char *hdr = (char *)_ehdr;
    bool is_64bits = hdr[EI_CLASS] == ELFCLASS64;

    if (is_64bits)
    {
        Elf64_Ehdr *ehdr = (Elf64_Ehdr *)hdr;
        Elf64_Shdr *shdr_table = (Elf64_Shdr *)(hdr + ehdr->e_shoff);
        Elf64_Shdr *shdr_str = &shdr_table[ehdr->e_shstrndx];
        char *shdr_str_table = hdr + shdr_str->sh_offset;

        for (size_t i = 0; i < ehdr->e_shnum; i++)
        {
            Elf64_Shdr *shdr = &shdr_table[i];
            char *shdr_name = shdr_str_table + shdr->sh_name;
            if (strcmp(shdr_name, name) == 0)
            {
                debug$("found section %s", name);
                return (void *)(hdr + shdr->sh_offset);
            }
        }

        if (!found)
        {
            error$("section %s not found", name);
            for (;;)
            {
                __asm__("hlt");
            }
        }
    }
    else
    {
        warn$("32-bits support is in the todo list");
        for (;;)
        {
            __asm__("hlt");
        }

        __builtin_unreachable();
    }

    __builtin_unreachable();
}