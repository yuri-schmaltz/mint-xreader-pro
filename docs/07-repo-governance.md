# Governança e Higiene de Repo: Xreader

Padrões para manter o repositório saudável e escalável.

## 1. Padrões de Código
Seguimos o **GNOME Programming Guidelines** e as diretrizes do Linux Mint.
- Indentação: Tabs (não espaços).
- Nomeação: `snake_case` para funções e variáveis, `CamelCase` para tipos de objetos GObject.

## 2. Organização de Pastas
- `shell/`: Código da UI principal.
- `libdocument/`: Interfaces de lógica de documento.
- `libview/`: Lógica de interface de visualização.
- `backend/`: Código específico por formato.
- `docs/`: Documentação técnica e operacional.

## 3. Política de Limpeza
- Arquivos gerados por build (`*.o`, `*.so`) nunca devem ser commitados (ver `.gitignore`).
- Arquivos grandes (>1MB) devem ser evitados no repo de código; use assets externos ou LFS se necessário.

## 5. Métricas de Sucesso (Baseline Jan 2026)
Para garantir a saúde contínua do projeto, os seguintes KPIs devem ser mantidos:
- **Onboarding Speed**: < 30 minutos do `git clone` até o primeiro build funcional.
- **Build Performance**: < 10 segundos para builds incrementais (Ninja).
- **Integridade de Docs**: 0 links quebrados e 100% de cobertura de novas funcionalidades.
- **Acessibilidade**: 100% de conformidade com a árvore AT-SPI para todos os novos widgets da shell.
