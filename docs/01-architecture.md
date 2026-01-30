# Arquitetura Técnica: Xreader

O Xreader é baseado em uma arquitetura robusta de **Backend / Documento / View**, permitindo a visualização de múltiplos formatos de forma agnóstica.

## 1. Modelo Core (Document/View)
- **libdocument**: Define as interfaces base para carregamento e renderização (`EvDocument`, `EvPage`).
- **libview**: Gerencia a renderização em tela, inputs de mouse/teclado e interações (`EvView`).
- **Backends**: Plugins independentes (`pdf`, `djvu`, `tiff`) que implementam as interfaces da `libdocument`.

## 2. Infraestrutura de Abas (Tab Infrastructure)
Implementado em Jan 2026 para permitir a visualização de múltiplos documentos em uma única janela.
- **Componente**: `GtkNotebook` em `shell/ev-window.c`.
- **Lógica de Instância Única**: O Xreader utiliza um daemon DBUS (`ev-application.c`) para interceptar novos chamados. Se uma janela já existir, o arquivo é aberto em uma nova aba via `ev_window_add_tab`.
- **Sincronização**: Cada aba mantém seu próprio contexto de `EvView` e `EvDocumentModel`.

## 3. Sistema de Anotações Avançadas (Edge-style)
Extensão das capacidades de anotação do Poppler/PDF.
- **Classes**: `EvAnnotationInk` (Desenho) e `EvAnnotationFreeText` (Texto Livre).
- **Interação**: 
  - `ev-view.c`: Gerencia a captura de coordenadas multi-ponto para desenhos.
  - Preview em tempo real: Implementado no loop de draw (`ev_view_draw`) para feedback visual instantâneo (Cyan line).
- **Backend Fallback**: Como algumas versões do Poppler possuem limitações de API, as anotações são convertidas/gerenciadas para garantir persistência mesmo em builds mais antigos.
