package hycallparser.builtinProcedures;

import hycallparser.parameter.Parameter;
import hycallparser.parameter.ParameterInteger;
import hycallparser.parameter.ParameterList;

import java.math.BigInteger;
import java.util.List;
import java.util.stream.Collectors;
import java.util.stream.LongStream;
import java.util.stream.Stream;

public class ProcedureRangeStep implements Procedure {

    @Override
    public String getName() {
        return "range";
    }

    @Override
    public int getParameterCount() {
        return 3;
    }

    @Override
    public Parameter perform(List<Parameter> args) {
        Parameter lower = args.get(0);
        Parameter step = args.get(1);
        Parameter upper = args.get(2);

        if (!(lower.isInteger() && step.isInteger() && upper.isInteger()))
            throw new IllegalArgumentException("Builtin procedure range expects three integers as parameters!");

        return new ParameterList(Stream.iterate(lower.getInt(), i -> i.compareTo(upper.getInt()) == -1, i -> i.add(step.getInt())).map(ParameterInteger::new).collect(Collectors.toList()));
    }
}
