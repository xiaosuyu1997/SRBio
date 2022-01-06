function rbwrite(filename, A, descr, key, opts)

if (nargin < 4 || nargout > 0 || nargin > 5)
    error('Usage: rbwrite(filename, A, descr, key[, opts])');
end

if (nargin < 5)
    opts = struct();
end

if ~issparse(A)
    error('Input matrix A must be sparse');
end

if ~isfield(opts, 'compress'); opts.compress = 'auto'; end
if ~isfield(opts, 'precision'); opts.precision = -1; end

compress = opts.compress;

% truncate descr and key
if numel(descr) > 72
    descr = descr(1:72);
end
if numel(key) > 8
    key = key(1:8);
end

% automatically detect whether A is symmetric
if (issymmetric(A))
    sym_flag = 1;
    A = tril(A); % only lower part of A is stored
else
    sym_flag = 0;
end

flag = 0;
[filedir, name, ext] = fileparts(filename);
s = what(filedir);
filename = fullfile(s.path, [name ext]);
switch (compress)
    case 'auto'
        if strcmp(ext, '.gz')
            flag = 1;
        elseif strcmp(ext, '.bz2')
            flag = 2;
        end
    case {'gzip', 'gz'}
        flag = 1;
    case {'bz2', 'bzip2'}
        flag = 2;
    otherwise
        error('Unknown compress mode %s', compress);
end

% call mex file
mex_srbio_write(filename, A, descr, key, opts.precision, sym_flag, flag);