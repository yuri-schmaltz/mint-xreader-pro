# Segurança e Privacidade: Xreader

O Xreader lida com arquivos de terceiros potencialmente maliciosos. A segurança é tratada como prioridade.

## 1. Modelo de Ameaça (Threat Model)
- **Vetor Principal**: Arquivos malformados (PDF, CBZ) explorando buffer overflows em backends.
- **Mitigação**: 
  - Uso de bibliotecas de sistema atualizadas (Poppler, Libarchive).
  - Sanitização de URIs antes do processamento.

## 2. Supply Chain
- Monitoramento de dependências via `debian/control`.
- Build determinístico no ambiente de CI oficial.

## 3. Privacidade (PII)
- O Xreader não envia dados para servidores externos.
- Metadados de documentos recentes são armazenados localmente no host.
- Histórico de documentos pode ser limpo via menu *Arquivo -> Limpar Histórico*.

## 4. Defaults Seguros
- O daemon `xreaderd` aceita conexões apenas via Session Bus local.
- Execução de scripts embutidos em documentos (ex: JavaScript em PDFs) é desativada por padrão quando possível.
