const fs = require('fs');

const contexts = [
  'TSMethodDefinition[accessibility=public]',
  'MethodDefinition[accessibility=public]',
  'TSParameterProperty[accessibility=public]',
  'TSPropertySignature',
  'PropertySignature',
  'TSInterfaceDeclaration',
  'InterfaceDeclaration',
  'TSPropertyDefinition[accessibility=public]',
  'PropertyDefinition[accessibility=public]',
  'TSTypeAliasDeclaration',
  'TypeAliasDeclaration',
  'TSTypeDeclaration',
  'TypeDeclaration',
  'TSEnumDeclaration',
  'EnumDeclaration',
  'TSClassDeclaration',
  'ClassDeclaration',
  'TSClassExpression',
  'ClassExpression',
  'TSFunctionExpression',
  'FunctionExpression',
  'TSInterfaceExpression',
  'InterfaceExpression',
  'TSEnumExpression',
  'EnumExpression',
];

function getFirstExisting(files) {
  for (const file of files) {
    if (fs.existsSync(file)) {
      return file;
    }
  }
  throw new Error('Found no existing file of: ' + files.join(', ') + ' at ' + process.cwd());
}

module.exports = {
  extends: ['eslint:recommended', 'plugin:@typescript-eslint/recommended', 'prettier'],
  root: true,
  parser: '@typescript-eslint/parser',
  plugins: ['@typescript-eslint', 'eslint-plugin-tsdoc', 'jsdoc'],
  overrides: [
    {
      files: ['*.ts', '*.tsx'],
      parserOptions: {
        // hacky workaround for CI not having the same tsconfig setup
        project: true,
      },
    },
    {
      files: '*.test.ts',
      rules: {
        'jsdoc/require-jsdoc': 'off',
      },
    },
  ],
  env: {
    node: true,
  },
  rules: {
    '@typescript-eslint/explicit-module-boundary-types': 'off',
    '@typescript-eslint/no-non-null-assertion': 'off',
    '@typescript-eslint/no-explicit-any': 'off',
    '@typescript-eslint/no-empty-function': 'off',
    '@typescript-eslint/await-thenable': 'error',
    '@typescript-eslint/no-floating-promises': 2,
    '@typescript-eslint/no-unused-vars': ['error', { argsIgnorePattern: '^_', varsIgnorePattern: '^_' }],
    'require-await': 2,
    'no-console': 'error',
    'no-constant-condition': 'off',
    camelcase: 2,
    'no-restricted-imports': [
      'error',
      {
        patterns: [
          {
            group: ['client-dest'],
            message: "Fix this absolute garbage import. It's your duty to solve it before it spreads.",
          },
          {
            group: ['dest'],
            message: 'You should not be importing from a build directory. Did you accidentally do a relative import?',
          },
        ],
      },
    ],
    'tsdoc/syntax': 'error',
    'jsdoc/require-jsdoc': [
      'error',
      {
        contexts,
        checkConstructors: false,
        checkGetters: true,
        checkSetters: true,
      },
    ],
    'jsdoc/require-description': ['error', { contexts }],
    'jsdoc/require-hyphen-before-param-description': ['error'],
    'jsdoc/require-param': ['error', { contexts, checkDestructured: false }],
    'jsdoc/require-param-description': ['error', { contexts }],
    'jsdoc/require-param-name': ['error', { contexts }],
    'jsdoc/require-property': ['error', { contexts }],
    'jsdoc/require-property-description': ['error', { contexts }],
    'jsdoc/require-property-name': ['error', { contexts }],
    'jsdoc/require-returns': ['error', { contexts }],
    'jsdoc/require-returns-description': ['error', { contexts }],
  },
  ignorePatterns: ['node_modules', 'dest*', 'dist', '*.js', '.eslintrc.cjs'],
};
