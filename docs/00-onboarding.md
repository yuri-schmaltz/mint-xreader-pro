# Onboarding: Xreader (Reborn)

Bem-vindo ao projeto Xreader! Este guia ajudará você a sair do zero até sua primeira contribuição em menos de 30 minutos.

## 1. Setup do Ambiente
### Dependências (Linux Mint/Ubuntu)
```bash
sudo apt install meson ninja-build libgtk-3-dev libpoppler-glib-dev libxapp-dev yelp-tools
```

## 2. Seu Primeiro Build
```bash
git clone https://github.com/yuri-schmaltz/mint-xreader.git
cd mint-xreader
meson setup builddir
ninja -C builddir
```

## 3. Rodando o Aplicativo
```bash
./builddir/shell/xreader
```

## 4. Onde Encontrar o Quê?
- **UI/Janela**: `shell/ev-window.c`
- **Desenho/Visualização**: `libview/ev-view.c`
- **Lógica de PDF**: `backend/pdf/ev-poppler.cc`
- **Anotações**: `libdocument/ev-annotation.c`

## 5. Troubleshooting de Setup
- **Meson não encontrado**: `pip install meson`
- **Erro de Linker (libxapp)**: Verifique se `libxapp-dev` está em uma versão >= 2.5.0.

Para mais detalhes técnicos, consulte o [Guia de Arquitetura](01-architecture.md).
