package hycallparser.parameter;

import java.math.BigInteger;

public class ParameterKeyValue extends Parameter {

    private String key;
    public Parameter value;

    public ParameterKeyValue(String key, Parameter value) {
        this.key = key;
        this.value = value;
    }

    @Override
    public boolean isKeyValue() {
        return true;
    }

    @Override
    public String getKey() {
        return key;
    }

    @Override
    public Parameter getValue() {
        return value;
    }

    @Override
    public String toString() {
        return "(" + key + " -> " + value.toString() + ")";
    }
}
