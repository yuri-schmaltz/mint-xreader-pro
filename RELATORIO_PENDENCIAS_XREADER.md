# Relatório de Pendências para Tornar o Xreader 10/10

## 1. Acessibilidade e Automação (ATK/AT-SPI)
- [ ] **Xreader não aparece na árvore de acessibilidade AT-SPI**
  - O binário está corretamente linkado com ATK, AT-SPI e atk-bridge.
  - O ambiente possui variáveis GTK_MODULES, DBUS, etc. corretas.
  - Serviços at-spi2 e dbus estão ativos e permissões corretas.
  - Testes com dogtail mostram que outros apps aparecem, mas xreader não.
  - Inicialização explícita do bridge ATK/AT-SPI já foi tentada no main.c, mas pode ser necessário:
    - Inicializar o bridge antes de qualquer chamada GTK (inclusive includes).
    - Garantir que o ambiente esteja correto no momento do launch (wrapper).
    - Investigar se há flags de build ou opções do Meson/GTK omitindo símbolos de acessibilidade.
  - **Ação sugerida:** Revisar ordem de inicialização, garantir ambiente, testar wrapper, revisar build.

## 2. Testes Automatizados de UI
- [ ] **Todos os testes de automação de UI falham**
  - Falha ao focar/clicar widgets: dogtail/pyatspi não encontra o xreader.
  - Depende da resolução do item 1.
  - **Ação sugerida:** Após resolver acessibilidade, reexecutar todos os testes.

## 3. Cobertura de Testes e QA
- [ ] **Cobertura de testes unitários e de integração**
  - Garantir que todos os fluxos críticos (abertura, navegação, busca, impressão, plugins) estejam cobertos.
  - Adicionar testes para fluxos de acessibilidade (leitor de tela, navegação por teclado, etc).
  - **Ação sugerida:** Mapear lacunas de cobertura e criar novos testes.

## 4. Performance e Otimização
- [ ] **Análise de performance em arquivos grandes e múltiplos formatos**
  - Medir tempo de abertura, navegação, busca e renderização.
  - Identificar gargalos em backends (PDF, EPUB, TIFF, etc).
  - **Ação sugerida:** Usar ferramentas de profiling e otimizar trechos críticos.

## 5. Internacionalização e Localização
- [ ] **Revisar strings, traduções e suporte a múltiplos idiomas**
  - Garantir que todas as mensagens estejam traduzidas e revisadas.
  - Validar suporte a idiomas RTL e caracteres especiais.
  - **Ação sugerida:** Revisar arquivos .po e fluxos de tradução.

## 6. Documentação e Experiência do Usuário
- [ ] **Documentação do usuário e do desenvolvedor**
  - Atualizar README, manuais, help in-app e documentação de API.
  - Garantir onboarding e dicas de acessibilidade para usuários.
  - **Ação sugerida:** Revisar e expandir documentação.

## 7. Empacotamento e Distribuição
- [ ] **Testar instalação e execução em diferentes distros e ambientes**
  - Validar dependências, scripts de instalação, integração com desktop.
  - Garantir que o app funcione em ambientes X11 e Wayland (se possível).
  - **Ação sugerida:** Testar em ambientes limpos e containers.

## 8. Segurança
- [ ] **Revisar manipulação de arquivos, permissões e sandboxing**
  - Garantir que arquivos temporários e dados sensíveis sejam tratados corretamente.
  - **Ação sugerida:** Revisar código e aplicar boas práticas de segurança.

---

## Resumo das Ações Prioritárias
1. Resolver registro do xreader no AT-SPI (acessibilidade).
2. Validar automação de UI e cobertura de testes.
3. Otimizar performance e revisar documentação.
4. Garantir empacotamento, internacionalização e segurança.

**Status atual:**
- Ambiente, dependências e serviços OK.
- Acessibilidade ainda não funcional para automação.
- Testes automatizados de UI todos falham por esse motivo.

**Próximos passos sugeridos:**
- Revisar inicialização do ATK/AT-SPI no código.
- Testar wrapper de ambiente para garantir variáveis.
- Reexecutar testes após ajustes.
- Mapear e cobrir lacunas de QA, performance e documentação.
