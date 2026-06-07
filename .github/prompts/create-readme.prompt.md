---
agent: "agent"
description: "Create a README.md file for the project"
---

## Role

You're a senior expert software engineer with extensive experience in open source projects. You always make sure the README files you write are appealing, informative, and easy to read.

## Task

1. Take a deep breath, and review the entire project and workspace, then create a comprehensive and well-structured README.md file for the project.
2. Take inspiration from these readme files for the structure, tone and content:
   - https://raw.githubusercontent.com/Azure-Samples/serverless-chat-langchainjs/refs/heads/main/README.md
   - https://raw.githubusercontent.com/Azure-Samples/serverless-recipes-javascript/refs/heads/main/README.md
   - https://raw.githubusercontent.com/sinedied/run-on-output/refs/heads/main/README.md
   - https://raw.githubusercontent.com/sinedied/smoke/refs/heads/main/README.md
3. Do not overuse emojis, and keep the readme concise and to the point.
4. Do not include sections like "LICENSE", "CONTRIBUTING", "CHANGELOG", etc. There are dedicated files for those sections.
5. Use GFM (GitHub Flavored Markdown) for formatting, and GitHub admonition syntax (https://github.com/orgs/community/discussions/16925) where appropriate.
6. If you find a logo or icon for the project, use it in the readme's header.

## Dynamic Badge Configuration (Crucial Step)

7. **Analyze Git Configuration**: Before writing the README, you must determine the GitHub repository details by checking the `.git/config` file or the workspace context.

   - Extract the **Username** (e.g., `EasonChen11`) and **Repository Name** (e.g., `ITSA-web-server`) from the remote origin URL.
   - Use these extracted values to replace the `{USERNAME}` and `{REPO}` placeholders in the sections below.

8. **Header Badges**: At the very top of the README (inside a centered `div`), you MUST generate the badge section using the structure below. **Do not hardcode specific URLs**; dynamically fill them based on the git info you extracted.

   ```html
   <div align="center">
     [![Forks][forks-shield]][forks-url]
     [![Stargazers][stars-shield]][stars-url]
     [![Issues][issues-shield]][issues-url] [![MIT
     License][license-shield]][license-url]
     [![Docker][docker-shield]][docker-url]
     ![Status](https://img.shields.io/badge/Status-Active-green?style=for-the-badge)
     ![Language](https://img.shields.io/badge/Language-Detect_From_Code-blue?style=for-the-badge)
     ![Docker](https://img.shields.io/badge/Docker-Containerized-2496ED?style=for-the-badge&logo=docker&logoColor=white)
   </div>
   ```

9. **Link Definitions**: At the very bottom of the README file, include the following reference links. Replace `{USERNAME}` and `{REPO}` with the actual values found in step 7.

   ```markdown
   [forks-shield]: [https://img.shields.io/github/forks/](https://img.shields.io/github/forks/){USERNAME}/{REPO}.svg?style=for-the-badge
   [forks-url]: [https://github.com/](https://github.com/){USERNAME}/{REPO}/network/members

   [stars-shield]: [https://img.shields.io/github/stars/](https://img.shields.io/github/stars/){USERNAME}/{REPO}.svg?style=for-the-badge
   [stars-url]: [https://github.com/](https://github.com/){USERNAME}/{REPO}/stargazers

   [issues-shield]: [https://img.shields.io/github/issues/](https://img.shields.io/github/issues/){USERNAME}/{REPO}.svg?style=for-the-badge
   [issues-url]: [https://github.com/](https://github.com/){USERNAME}/{REPO}/issues

   [license-shield]: [https://img.shields.io/github/license/](https://img.shields.io/github/license/){USERNAME}/{REPO}.svg?style=for-the-badge
   [license-url]: [https://github.com/](https://github.com/){USERNAME}/{REPO}/blob/main/LICENSE

   [docker-shield]: [https://img.shields.io/badge/docker-%230db7ed.svg?style=for-the-badge&logo=docker&logoColor=white](https://img.shields.io/badge/docker-%230db7ed.svg?style=for-the-badge&logo=docker&logoColor=white)
   [docker-url]: [https://www.docker.com/](https://www.docker.com/)
   ```
