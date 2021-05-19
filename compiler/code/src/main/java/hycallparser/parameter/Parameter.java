package hycallparser.parameter;

import java.math.BigInteger;
import java.util.List;

public abstract class Parameter {

    public boolean isList() {
        return false;
    }

    public boolean isKeyValue() {
        return false;
    }

    public boolean isInteger() {
        return false;
    }

    public boolean isString() {
        return false;
    }

    public BigInteger getInt() {
        throw new UnsupportedOperationException();
    }

    public String getString() {
        throw new UnsupportedOperationException();
    }

    public String getKey() {
        throw new UnsupportedOperationException();
    }
    public Parameter getValue() {
        throw new UnsupportedOperationException();
    }

    public List<Parameter> getValues() {
        throw new UnsupportedOperationException();
    }
}
