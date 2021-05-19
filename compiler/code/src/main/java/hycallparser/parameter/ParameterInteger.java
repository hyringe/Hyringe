package hycallparser.parameter;

import java.math.BigInteger;

public class ParameterInteger extends Parameter {

    private BigInteger value;

    public ParameterInteger(BigInteger value) {
        this.value = value;
    }

    @Override
    public boolean isInteger() {
        return true;
    }

    @Override
    public BigInteger getInt() {
        return value;
    }

    @Override
    public String toString() {
        return value.toString();
    }
}
