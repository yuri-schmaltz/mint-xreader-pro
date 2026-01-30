# Operação e Runbooks: Xreader

## 1. Build e Desenvolvimento
O Xreader utiliza **Meson** e **Ninja**.

### Setup Inicial (Local)
```bash
meson setup builddir --prefix=/usr/local --buildtype=debugoptimized
ninja -C builddir
```

### Execução para Debug
Para ver logs detalhados do backend e da visualização:
```bash
G_MESSAGES_DEBUG=all builddir/shell/xreader document.pdf
```

## 2. Testes (QA)
### Testes de Interface (Dogtail/AT-SPI)
```bash
# Requisito: python3-dogtail
python3 test/testFileMenu.py ./builddir/shell/xreader
```

## 3. Troubleshooting Comum
- **Erro de Renderização**: Verificar se o backend correto está sendo carregado via `EV_BACKENDS_DIR`.
- **Abas não aparecem**: Certifique-se de que o daemon `xreaderd` não está em conflito com versões antigas.
- **Crash em Anotação**: Verifique se o documento tem permissão de escrita e se o Poppler GLib está atualizado.

## 4. Performance
O baseline de performance é monitorado via logs de tempo no startup.
- **Meta de Startup**: < 150ms para abertura de PDF.
- **Cache**: O cache de pixbufs é limitado dinamicamente para evitar consumo excessivo de RAM (ver `libview/ev-pixbuf-cache.c`).
