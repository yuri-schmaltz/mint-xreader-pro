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

### Execução Dev Completa (com daemon DBus)
Para rodar em ambiente de desenvolvimento com sessão DBus isolada e `xreaderd`:
```bash
install-scripts/dev-run.sh test/test-links.pdf
```
Por padrão, logs da sessão DBus vão para `build/dev-run/session-*.log`.
Para modo verboso no terminal:
```bash
DEV_RUN_VERBOSE=1 install-scripts/dev-run.sh test/test-links.pdf
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

## 5. Diagnostics Reprodutível (HOP)
Para coletar baseline operacional/performance de forma consistente:

```bash
install-scripts/diagnostics.sh
```

Artefatos gerados:
- `build/hop-diagnostics/report-<timestamp>.txt`
- `build/hop-diagnostics/run-<timestamp>.out|.err`
- `build/hop-diagnostics/smoke-<timestamp>.out|.err`

O relatório inclui:
- tempo de startup (`--help`) em 5 amostras
- smoke test com timeout controlado
- estimativas de `first_result` por timestamps de log
- snapshot de CPU/RAM (processo principal + child)
- contagem de warnings/criticals/errors
- footprint de disco e versões de dependências
