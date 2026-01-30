# Release e Versionamento: Xreader

O Xreader segue o ciclo de desenvolvimento do Linux Mint, mas mantém tags de versão semântica.

## 1. Versionamento
- **Major**: Mudanças arquiteturais (ex: Migração GTK4).
- **Minor**: Novas funcionalidades (ex: Sistema de Abas).
- **Patch**: Bugfixes e traduções.

## 2. Fluxo de Empacotamento Debian
Para gerar o arquivo `.deb` para distribuição:
```bash
# Dentro da raiz do repositório
dpkg-buildpackage -us -uc -b
```
Os artefatos serão gerados no diretório pai.

## 3. Changelog
O arquivo `debian/changelog` é a fonte da verdade para o histórico de versões distribuídas. Use `dch -i` para adicionar novas entradas durante o desenvolvimento.

## 4. Rollback
Em caso de falha crítica em produção:
1. Reverter o commit problemático no branch `master`.
2. Incrementar o patch level e gerar uma nova release "hotfix".
